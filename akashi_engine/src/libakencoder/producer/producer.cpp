#include "./producer.h"
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

#include <GLFW/glfw3.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ProduceLoopContext ctx;
            eval::AKEval* eval = nullptr;
        };
        struct EncodeContext {
            Rational cur_pts;
            Rational fps;
            Rational duration;
            core::Path entry_path{""};
            core::owned_ptr<codec::AKDecoder> decoder;
            core::owned_ptr<buffer::AVBuffer> buffer;

            bool decode_ended = false;
        };

        static void exit_thread(ExitContext& exit_ctx) {
            if (exit_ctx.eval) {
                exit_ctx.eval->exit();
            }
            exit_ctx.ctx.state->set_producer_finished(true, true);
        }

        static EncodeContext create_encode_context(ProduceLoopContext& ctx,
                                                   core::borrowed_ptr<eval::AKEval> eval) {
            Rational start_pts = Rational(0, 1);
            Rational fps;
            core::Path entry_path{""};
            RenderProfile profile;
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
                fps = ctx.state->m_prop.fps;
                profile = eval->render_prof(entry_path.to_abspath().to_str());

                ctx.state->m_prop.render_prof = profile;
            }

            return {.cur_pts = start_pts,
                    .fps = fps,
                    .duration = to_rational(profile.duration),
                    .entry_path = entry_path,
                    .decoder = make_owned<codec::AKDecoder>(profile.atom_profiles, start_pts),
                    .buffer = make_owned<buffer::AVBuffer>(borrowed_ptr(ctx.state))};
        }

        static void update_encode_context(EncodeContext& encode_ctx) {
            encode_ctx.cur_pts += (Rational(1, 1) / encode_ctx.fps);
        }

        static bool can_produce(const EncodeContext& encode_ctx) {
            // [TODO] exclusive?
            return encode_ctx.cur_pts <= encode_ctx.duration;
        }

        static bool exec_decode(ProduceLoopContext& ctx, EncodeContext& encode_ctx) {
            if (encode_ctx.decode_ended) {
                return true;
            }

            codec::DecodeArg decode_args;
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                decode_args.out_audio_spec = ctx.state->m_atomic_state.audio_spec.load();
                decode_args.decode_method = ctx.state->m_atomic_state.decode_method.load();
                // [TODO] this value should be changed
                decode_args.video_max_queue_count = ctx.state->m_prop.video_max_queue_count;
            }

            while (true) {
                if (!ctx.state->get_video_decode_ready() || !ctx.state->get_audio_decode_ready()) {
                    break;
                }
                auto decode_res = encode_ctx.decoder->decode(decode_args);

                switch (decode_res.result) {
                    case codec::DecodeResultCode::ERROR: {
                        AKLOG_ERROR("decode error, code: {}", decode_res.result);
                        return false;
                    }
                    case codec::DecodeResultCode::DECODE_ENDED: {
                        AKLOG_INFO("DecodeLoop::decode_thread(): ended, code: {}",
                                   decode_res.result);
                        encode_ctx.decode_ended = true;
                        return true;
                    }
                    case codec::DecodeResultCode::DECODE_LAYER_EOF:
                    case codec::DecodeResultCode::DECODE_LAYER_ENDED:
                    case codec::DecodeResultCode::DECODE_STREAM_ENDED:
                    case codec::DecodeResultCode::DECODE_ATOM_ENDED:
                    case codec::DecodeResultCode::DECODE_AGAIN:
                    case codec::DecodeResultCode::DECODE_SKIPPED: {
                        AKLOG_INFO("decode skipped or layer ended, code: {}, uuid: {}",
                                   decode_res.result, decode_res.layer_uuid);
                        break;
                    }
                    case codec::DecodeResultCode::OK: {
                        switch (decode_res.buffer->prop().media_type) {
                            case buffer::AVBufferType::VIDEO: {
                                const auto comp_layer_uuid =
                                    decode_res.layer_uuid + std::to_string(0);
                                encode_ctx.buffer->vq->enqueue(comp_layer_uuid,
                                                               std::move(decode_res.buffer));
                                break;
                            }
                            case buffer::AVBufferType::AUDIO: {
                                const auto comp_layer_uuid =
                                    decode_res.layer_uuid + std::to_string(0);
                                encode_ctx.buffer->aq->enqueue(comp_layer_uuid,
                                                               std::move(decode_res.buffer));
                                break;
                            }
                            default: {
                            }
                        }

                        break;
                    }
                    default: {
                        AKLOG_ERROR("invalid result code found, code: {}",
                                    static_cast<int>(decode_res.result));
                        return false;
                    }
                }
            }
            return true;
        }

        static std::vector<core::FrameContext>
        pull_frame_context(core::borrowed_ptr<eval::AKEval> eval, const EncodeContext& encode_ctx) {
            return eval->eval_krons(encode_ctx.entry_path.to_abspath().to_str(), encode_ctx.cur_pts,
                                    encode_ctx.fps.to_decimal(), encode_ctx.duration, 1);
        }

        void ProduceLoop::produce_thread(ProduceLoopContext ctx, ProduceLoop* loop) {
            AKLOG_INFON("Producer init");

            glfwInit();

            auto eval = make_owned<eval::AKEval>(borrowed_ptr(ctx.state));

            ExitContext exit_ctx{ctx, eval.get()};

            loop->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    exit_thread(*exit_ctx_);
                    AKLOG_INFON("Producer Successfully exited");
                },
                &exit_ctx);

            // enqueue data until all frames processed
            for (
                /* clang-format off */
                auto encode_ctx = create_encode_context(ctx, borrowed_ptr(eval)); 
                can_produce(encode_ctx);
                update_encode_context(encode_ctx)
                /* clang-format on */
            ) {
                ctx.queue->wait_for_not_full();

                // eval
                auto frame_ctx = pull_frame_context(borrowed_ptr(eval), encode_ctx);

                // decode
                if (!exec_decode(ctx, encode_ctx)) {
                    break;
                }

                // render

                // enqueue
                ctx.queue->enqueue({encode_ctx.cur_pts.to_decimal()});
            }

            exit_thread(exit_ctx);
            AKLOG_INFON("Producer finished");
        }

    }
}
