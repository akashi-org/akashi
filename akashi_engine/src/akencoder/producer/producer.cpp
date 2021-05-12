#include "./producer.h"
#include "./window.h"
#include "./decoder.h"
#include "./audio_renderer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/path.h>
#include <libakstate/akstate.h>
#include <libakeval/akeval.h>
#include <libakcodec/akcodec.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>
#include <libakbuffer/audio_queue.h>
#include <libakgraphics/akgraphics.h>
#include <libakgraphics/item.h>
#include <libakcodec/encoder.h>

#include <libakbuffer/audio_buffer.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ProduceLoopContext ctx;
            ProduceLoop* loop;
            eval::AKEval* eval = nullptr;
        };

        struct EncodeContext {
            core::RenderProfile render_profile;
            Rational cur_pts;
            Rational next_pts;
            Rational fps;
            Rational duration;
            int video_width;
            int video_height;
            core::Path entry_path{""};
            std::string elem_name{""};
            core::owned_ptr<codec::AKDecoder> decoder;
            core::owned_ptr<buffer::AVBuffer> buffer;

            core::owned_ptr<buffer::AudioBuffer> abuffer;

            core::Rational audio_encode_pts = core::Rational(-1, 1);

            core::owned_ptr<graphics::AKGraphics> gfx;
            graphics::EncodeRenderParams er_params;
            core::owned_ptr<Window> window;

            bool decode_ended = false;

          public:
            ~EncodeContext() {
                // if (this->er_params.buffer) {
                //     delete this->er_params.buffer;
                // }
            }
        };

        static core::owned_ptr<EncodeContext>
        create_encode_context(ProduceLoopContext& ctx, core::borrowed_ptr<eval::AKEval> eval) {
            Rational start_pts = Rational(0, 1);
            Rational fps;
            core::Path entry_path{""};
            std::string elem_name{""};
            RenderProfile profile;
            int video_width = -1;
            int video_height = -1;
            core::AKAudioSpec encode_audio_spec;
            size_t audio_max_queue_size = 1;
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
                elem_name = ctx.state->m_prop.eval_state.config.elem_name;
                fps = ctx.state->m_prop.fps;
                profile = eval->render_prof(entry_path.to_abspath().to_str(), elem_name);
                video_width = ctx.state->m_prop.video_width;
                video_height = ctx.state->m_prop.video_height;

                ctx.state->m_prop.render_prof = profile;

                encode_audio_spec = ctx.state->m_atomic_state.encode_audio_spec.load();
                audio_max_queue_size = ctx.state->m_prop.audio_max_queue_size;
            }

            core::owned_ptr<EncodeContext> encode_ctx{new EncodeContext};

            encode_ctx->render_profile = profile;
            encode_ctx->cur_pts = start_pts;
            encode_ctx->next_pts = start_pts;
            encode_ctx->fps = fps;
            encode_ctx->duration = to_rational(profile.duration);
            encode_ctx->video_width = video_width;
            encode_ctx->video_height = video_height;
            encode_ctx->entry_path = entry_path;
            encode_ctx->elem_name = elem_name;
            encode_ctx->decoder = make_owned<codec::AKDecoder>(profile.atom_profiles, start_pts);
            encode_ctx->buffer = make_owned<buffer::AVBuffer>(borrowed_ptr(ctx.state));
            encode_ctx->abuffer =
                make_owned<buffer::AudioBuffer>(encode_audio_spec, audio_max_queue_size);
            encode_ctx->gfx = nullptr;
            encode_ctx->window = make_owned<Window>();

            return encode_ctx;
        }

        static void init_encode_context(ProduceLoopContext& ctx, EncodeContext& encode_ctx) {
            encode_ctx.gfx =
                make_owned<graphics::AKGraphics>(ctx.state, borrowed_ptr(encode_ctx.buffer));
            encode_ctx.gfx->load_api({Window::get_proc_address}, {Window::egl_get_proc_address});
            encode_ctx.gfx->load_fbo(encode_ctx.render_profile, true);
        }

        static void update_encode_context(EncodeContext& encode_ctx) {
            // loop detected
            if (encode_ctx.cur_pts >= encode_ctx.next_pts) {
                // incr cur_pts to terminate the produce session
                encode_ctx.cur_pts += (Rational(1, 1) / encode_ctx.fps);
            } else {
                encode_ctx.cur_pts = encode_ctx.next_pts;
            }
        }

        static bool can_produce(const EncodeContext& encode_ctx) {
            return encode_ctx.cur_pts <= encode_ctx.duration;
        }

        static std::vector<core::FrameContext>
        pull_frame_context(core::borrowed_ptr<eval::AKEval> eval, const EncodeContext& encode_ctx) {
            return eval->eval_krons(encode_ctx.entry_path.to_abspath().to_str(), encode_ctx.cur_pts,
                                    encode_ctx.fps.to_decimal(), encode_ctx.duration, 2);
        }

        void ProduceLoop::produce_thread(ProduceLoopContext ctx, ProduceLoop* loop) {
            AKLOG_INFON("Producer init");

            auto eval = make_owned<eval::AKEval>(borrowed_ptr(ctx.state));

            ExitContext exit_ctx{ctx, loop, eval.get()};

            loop->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    ProduceLoop::exit_thread(*exit_ctx_);
                    AKLOG_INFON("Producer Successfully exited");
                },
                &exit_ctx);

            // enqueue data until all frames processed

            auto encode_ctx = create_encode_context(ctx, borrowed_ptr(eval));
            init_encode_context(ctx, *encode_ctx);

            auto nb_samples_per_frame = ctx.encoder->nb_samples_per_frame();
            // [TODO] maybe we should need an assertion that audio buffer size is grater than or
            // equal to the value of nb_samples_per_frame

            DecodeParams decode_params = {
                borrowed_ptr(ctx.state), borrowed_ptr(encode_ctx->decoder),
                borrowed_ptr(encode_ctx->buffer), borrowed_ptr(encode_ctx->abuffer)};

            for (; can_produce(*encode_ctx); update_encode_context(*encode_ctx)) {
                ctx.queue->wait_for_not_full();

                // eval
                auto frame_ctx = pull_frame_context(borrowed_ptr(eval), *encode_ctx);
                if (frame_ctx.empty()) {
                    break;
                }
                if (frame_ctx.size() < 2) {
                    AKLOG_ERROR("got only {} counts from evaluation", frame_ctx.size());
                    break;
                }
                encode_ctx->cur_pts = to_rational(frame_ctx[0].pts);
                encode_ctx->next_pts = to_rational(frame_ctx[1].pts);

                // decode
                if (!encode_ctx->decode_ended) {
                    auto decode_result = exec_decode(decode_params);
                    if (decode_result == DecodeResult::ERR) {
                        break;
                    } else if (decode_result == DecodeResult::ENDED) {
                        encode_ctx->decode_ended = true;
                    }
                }

                // video render
                if (ctx.state->m_encode_conf.video_codec != core::EncodeCodec::NONE) {
                    // glfwMakeContextCurrent(encode_ctx->window);
                    encode_ctx->er_params.buffer =
                        new uint8_t[encode_ctx->video_width * encode_ctx->video_height * 3];
                    encode_ctx->gfx->encode_render(encode_ctx->er_params, frame_ctx[0]);
                    // glfwSwapBuffers(encode_ctx->window);

                    EncodeQueueData queue_data;
                    queue_data.pts = encode_ctx->cur_pts;
                    queue_data.buffer.reset(encode_ctx->er_params.buffer);
                    queue_data.buf_size = encode_ctx->video_width * encode_ctx->video_height * 3;
                    queue_data.type = buffer::AVBufferType::VIDEO;

                    ctx.queue->enqueue(std::move(queue_data));
                }

                // audio render
                if (ctx.state->m_encode_conf.audio_codec != core::EncodeCodec::NONE) {
                    auto datasets =
                        render_audio(&encode_ctx->audio_encode_pts, borrowed_ptr(ctx.state),
                                     borrowed_ptr(encode_ctx->abuffer), nb_samples_per_frame,
                                     encode_ctx->cur_pts);

                    for (auto&& dataset : datasets) {
                        ctx.queue->enqueue(std::move(dataset));
                    }
                }
            }

            ProduceLoop::exit_thread(exit_ctx);
            AKLOG_INFON("Producer finished");
        }

        void ProduceLoop::exit_thread(ExitContext& exit_ctx) {
            if (exit_ctx.eval) {
                exit_ctx.eval->exit();
            }
            exit_ctx.loop->m_thread_exited = true;
            exit_ctx.ctx.state->set_producer_finished(true, true);
        }
    }
}
