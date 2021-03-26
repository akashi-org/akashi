#include "./producer.h"
#include "./window.h"
#include "./decoder.h"
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
            Rational fps;
            Rational duration;
            int video_width;
            int video_height;
            core::Path entry_path{""};
            core::owned_ptr<codec::AKDecoder> decoder;
            core::owned_ptr<buffer::AVBuffer> buffer;

            core::owned_ptr<buffer::AudioBuffer> abuffer;

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

        static EncodeContext create_encode_context(ProduceLoopContext& ctx,
                                                   core::borrowed_ptr<eval::AKEval> eval) {
            Rational start_pts = Rational(0, 1);
            Rational fps;
            core::Path entry_path{""};
            RenderProfile profile;
            int video_width = -1;
            int video_height = -1;
            core::AKAudioSpec encode_audio_spec;
            size_t audio_max_queue_size = 1;
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
                fps = ctx.state->m_prop.fps;
                profile = eval->render_prof(entry_path.to_abspath().to_str());
                video_width = ctx.state->m_prop.video_width;
                video_height = ctx.state->m_prop.video_height;

                ctx.state->m_prop.render_prof = profile;

                encode_audio_spec = ctx.state->m_atomic_state.encode_audio_spec.load();
                audio_max_queue_size = ctx.state->m_prop.audio_max_queue_size;
            }

            return {.render_profile = profile,
                    .cur_pts = start_pts,
                    .fps = fps,
                    .duration = to_rational(profile.duration),
                    .video_width = video_width,
                    .video_height = video_height,
                    .entry_path = entry_path,
                    .decoder = make_owned<codec::AKDecoder>(profile.atom_profiles, start_pts),
                    .buffer = make_owned<buffer::AVBuffer>(borrowed_ptr(ctx.state)),
                    .abuffer =
                        make_owned<buffer::AudioBuffer>(encode_audio_spec, audio_max_queue_size),
                    .gfx = nullptr,
                    .window = make_owned<Window>()};
        }

        static void init_encode_context(ProduceLoopContext& ctx, EncodeContext& encode_ctx) {
            encode_ctx.gfx =
                make_owned<graphics::AKGraphics>(ctx.state, borrowed_ptr(encode_ctx.buffer));
            encode_ctx.gfx->load_api({Window::get_proc_address}, {Window::egl_get_proc_address});
            encode_ctx.gfx->load_fbo(encode_ctx.render_profile, true);
        }

        static void update_encode_context(EncodeContext& encode_ctx) {
            encode_ctx.cur_pts += (Rational(1, 1) / encode_ctx.fps);
        }

        static bool can_produce(const EncodeContext& encode_ctx) {
            // [TODO] exclusive?
            return encode_ctx.cur_pts <= encode_ctx.duration;
        }

        static std::vector<core::FrameContext>
        pull_frame_context(core::borrowed_ptr<eval::AKEval> eval, const EncodeContext& encode_ctx) {
            return eval->eval_krons(encode_ctx.entry_path.to_abspath().to_str(), encode_ctx.cur_pts,
                                    encode_ctx.fps.to_decimal(), encode_ctx.duration, 1);
        }

        static core::Rational before_pts = core::Rational(-1, 1);

        static std::vector<EncodeQueueData> audio_buffers(const ProduceLoopContext& ctx,
                                                          const size_t nb_samples_per_frame,
                                                          const core::Rational& max_pts) {
            std::vector<EncodeQueueData> datasets;

            auto audio_spec = ctx.state->m_atomic_state.audio_spec.load();
            auto audio_buf_size =
                nb_samples_per_frame * core::size_table(audio_spec.format) * audio_spec.channels;
            auto frame_dur = core::Rational(audio_buf_size, core::bytes_per_second(audio_spec));

            while (true) {
                auto frame_pts =
                    before_pts < Rational(0, 1) ? Rational(0, 1) : frame_dur + before_pts;
                if (frame_pts > max_pts) {
                    break;
                }
                EncodeQueueData queue_data;
                queue_data.pts = frame_pts;
                queue_data.buf_size = audio_buf_size;
                queue_data.buffer.reset(new uint8_t[queue_data.buf_size]());
                queue_data.nb_samples = nb_samples_per_frame;
                queue_data.type = buffer::AVBufferType::AUDIO;
                datasets.push_back(std::move(queue_data));

                before_pts = frame_pts;
            }

            return datasets;
        }

        static std::vector<EncodeQueueData> audio_buffers2(const ProduceLoopContext& ctx,
                                                           EncodeContext& encode_ctx,
                                                           const size_t nb_samples_per_frame,
                                                           const core::Rational& max_pts) {
            std::vector<EncodeQueueData> datasets;

            auto audio_spec = ctx.state->m_atomic_state.audio_spec.load();
            auto audio_buf_size =
                nb_samples_per_frame * core::size_table(audio_spec.format) * audio_spec.channels;
            auto frame_dur = core::Rational(audio_buf_size, core::bytes_per_second(audio_spec));

            while (true) {
                auto frame_pts =
                    before_pts < Rational(0, 1) ? Rational(0, 1) : frame_dur + before_pts;
                if (frame_pts > max_pts) {
                    break;
                }
                EncodeQueueData queue_data;
                queue_data.pts = frame_pts;
                queue_data.buf_size = audio_buf_size;
                queue_data.buffer.reset(new uint8_t[queue_data.buf_size]());
                queue_data.nb_samples = nb_samples_per_frame;
                queue_data.type = buffer::AVBufferType::AUDIO;

                auto result =
                    encode_ctx.abuffer->dequeue(queue_data.buffer.get(), queue_data.buf_size);
                if (result == buffer::AudioBuffer::Result::OUT_OF_RANGE) {
                    auto before_pts = encode_ctx.abuffer->cur_pts();
                    encode_ctx.abuffer->seek(queue_data.buf_size);
                    AKLOG_ERROR("AudioBuffer Seeked {} => {}", before_pts.to_decimal(),
                                encode_ctx.abuffer->cur_pts().to_decimal());
                    continue;
                } else if (result != buffer::AudioBuffer::Result::OK) {
                    AKLOG_ERROR("Got invalid result {}", result);
                    // what should we do here?
                }

                AKLOG_INFO("AudioBuffer Dequeued, pts: {}", frame_pts.to_decimal());

                datasets.push_back(std::move(queue_data));

                before_pts = frame_pts;
            }

            return datasets;
        }

        static bool needs_finit = true;
        void save_pcm(uint8_t* buf, size_t buf_size, const char* prefix) {
            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "audio_%s.buf", prefix);

            FILE* f;

            if (needs_finit) {
                remove("audio_left.buf");
                remove("audio_right.buf");
                needs_finit = false;
            }

            f = fopen(frame_filename, "ab");
            fwrite(buf, 1, static_cast<size_t>(buf_size), f);
            fclose(f);
        };

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
            init_encode_context(ctx, encode_ctx);

            auto nb_samples_per_frame = ctx.encoder->nb_samples_per_frame();
            // [TODO] maybe we should need an assertion that audio buffer size is grater than or
            // equal to the value of nb_samples_per_frame

            DecodeParams decode_params = {borrowed_ptr(ctx.state), borrowed_ptr(encode_ctx.decoder),
                                          borrowed_ptr(encode_ctx.buffer),
                                          borrowed_ptr(encode_ctx.abuffer)};

            for (; can_produce(encode_ctx); update_encode_context(encode_ctx)) {
                ctx.queue->wait_for_not_full();

                // eval
                auto frame_ctx = pull_frame_context(borrowed_ptr(eval), encode_ctx);

                // decode
                if (!encode_ctx.decode_ended) {
                    auto decode_result = exec_decode(decode_params);
                    if (decode_result == DecodeResult::ERR) {
                        break;
                    } else if (decode_result == DecodeResult::ENDED) {
                        encode_ctx.decode_ended = true;
                    }
                }

                // video render
                if (ctx.state->m_encode_conf.video_codec != core::EncodeCodec::NONE) {
                    // glfwMakeContextCurrent(encode_ctx.window);
                    encode_ctx.er_params.buffer =
                        new uint8_t[encode_ctx.video_width * encode_ctx.video_height * 3];
                    encode_ctx.gfx->encode_render(encode_ctx.er_params, frame_ctx[0]);
                    // glfwSwapBuffers(encode_ctx.window);

                    EncodeQueueData queue_data;
                    queue_data.pts = encode_ctx.cur_pts;
                    queue_data.buffer.reset(encode_ctx.er_params.buffer);
                    queue_data.buf_size = encode_ctx.video_width * encode_ctx.video_height * 3;
                    queue_data.type = buffer::AVBufferType::VIDEO;

                    ctx.queue->enqueue(std::move(queue_data));
                }

                // audio render
                if (ctx.state->m_encode_conf.audio_codec != core::EncodeCodec::NONE) {
                    auto datasets =
                        audio_buffers2(ctx, encode_ctx, nb_samples_per_frame, encode_ctx.cur_pts);
                    // for (auto&& dataset : datasets) {
                    //     save_pcm(dataset.buffer.get(), dataset.buf_size / 2, "left");
                    //     save_pcm(dataset.buffer.get() + dataset.buf_size / 2,
                    //     dataset.buf_size / 2,
                    //              "right");
                    // }
                    // datasets = audio_buffers(ctx, nb_samples_per_frame, encode_ctx.cur_pts);

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
