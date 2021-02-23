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
#include <libakgraphics/akgraphics.h>
#include <libakgraphics/item.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
// #define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_EGL
#include <GLFW/glfw3native.h>

#include <EGL/egl.h>

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

            core::owned_ptr<graphics::AKGraphics> gfx;
            graphics::EncodeRenderParams er_params;
            GLFWwindow* window = nullptr;

            bool decode_ended = false;

          public:
            ~EncodeContext() {
                if (this->window) {
                    glfwDestroyWindow(this->window);
                    glfwTerminate();
                }
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
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
                fps = ctx.state->m_prop.fps;
                profile = eval->render_prof(entry_path.to_abspath().to_str());
                video_width = ctx.state->m_prop.video_width;
                video_height = ctx.state->m_prop.video_height;

                ctx.state->m_prop.render_prof = profile;
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
                    .gfx = nullptr};
        }

        static void* get_proc_address(void*, const char* name) {
            return reinterpret_cast<void*>(glfwGetProcAddress(name));
        }

        static void* egl_get_proc_address(void*, const char* name) {
            // [TODO] add checks for validity of egl?
            return reinterpret_cast<void*>(eglGetProcAddress(name));
        }

        static void init_encode_context(ProduceLoopContext& ctx, EncodeContext& encode_ctx) {
            glfwInit();
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            // on linux/x11 system, force egl over glx
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

            // glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            // glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);

            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            encode_ctx.window = glfwCreateWindow(640, 480, "", NULL, NULL);

            glfwMakeContextCurrent(encode_ctx.window);

            encode_ctx.gfx =
                make_owned<graphics::AKGraphics>(ctx.state, borrowed_ptr(encode_ctx.buffer));
            encode_ctx.gfx->load_api({get_proc_address}, {egl_get_proc_address});
            encode_ctx.gfx->load_fbo(encode_ctx.render_profile, true);
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
                                // const auto comp_layer_uuid =
                                //     decode_res.layer_uuid + std::to_string(0);
                                // encode_ctx.buffer->aq->enqueue(comp_layer_uuid,
                                //                                std::move(decode_res.buffer));
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

            for (; can_produce(encode_ctx); update_encode_context(encode_ctx)) {
                ctx.queue->wait_for_not_full();

                // eval
                auto frame_ctx = pull_frame_context(borrowed_ptr(eval), encode_ctx);

                // decode
                if (!exec_decode(ctx, encode_ctx)) {
                    break;
                }

                // render
                // glfwMakeContextCurrent(encode_ctx.window);
                encode_ctx.er_params.buffer =
                    new uint8_t[encode_ctx.video_width * encode_ctx.video_height * 3];
                encode_ctx.gfx->encode_render(encode_ctx.er_params, frame_ctx[0]);
                // glfwSwapBuffers(encode_ctx.window);

                // enqueue
                EncodeQueueData queue_data;
                queue_data.pts = encode_ctx.cur_pts;
                queue_data.buffer.reset(encode_ctx.er_params.buffer);
                queue_data.buf_size = encode_ctx.video_width * encode_ctx.video_height * 3;
                queue_data.type = buffer::AVBufferType::VIDEO;

                ctx.queue->enqueue(std::move(queue_data));
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
