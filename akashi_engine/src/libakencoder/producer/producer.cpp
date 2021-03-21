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
#include <libakcodec/encoder.h>

#include <libakbuffer/ringbuffer.h>

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
        class AudioBuffer final {
          public:
            struct Data {
                uint8_t* buf = nullptr;
                size_t len = 0;
            };
            enum class Result {
                OUT_OF_RANGE = -2,
                ERR = -1,
                NONE = 0,
                OK = 1,
            };

          public:
            explicit AudioBuffer(const core::AKAudioSpec& spec, const size_t max_bufsize) {
                for (int i = 0; i < spec.channels; i++) {
                    m_buffers.push_back(
                        make_owned<buffer::AudioRingBuffer2>(max_bufsize / spec.channels, spec));
                }
            };
            virtual ~AudioBuffer(){};

            // if failed, input buffer is to be stored in m_back_buffer
            // precondition: write_ready() returns true
            AudioBuffer::Result enqueue(core::owned_ptr<buffer::AVBufferData> buf_data) {
                if (m_back_buffer) {
                    auto back_buffer = std::move(m_back_buffer);
                    m_back_buffer = nullptr;
                    // [XXX] here enqueue() must not be called when m_back_buffer remains filled
                    auto res = this->enqueue(std::move(back_buffer));
                    // this result is unexpected, but we handle it just in case
                    if (res != AudioBuffer::Result::OK) {
                        AKLOG_ERROR("Got invalid ResultCode {}", res);
                        return AudioBuffer::Result::ERR;
                    }
                }

#ifndef NDEBUG
                if (buf_data->prop().sample_format != core::AKAudioSampleFormat::FLTP) {
                    AKLOG_ERROR("sample format is not FLTP, {}", buf_data->prop().sample_format);
                    return AudioBuffer::Result::ERR;
                }
#endif
                auto wbufs = this->to_buffers(*buf_data);

                for (size_t i = 0; i < wbufs.size(); i++) {
                    if (!m_buffers[i]->write(wbufs[i].buf, wbufs[i].len, buf_data->prop().pts,
                                             true)) {
                        m_back_buffer = std::move(buf_data);
                        return AudioBuffer::Result::OUT_OF_RANGE;
                    }
                }
                return AudioBuffer::Result::OK;
            }

            /**
             *
             * @params (buf) a 1-D planar audio buffer
             * @params (buf_size) buf length, not in bytes
             *
             * @detail
             * precondition: `buf` is properly allocated
             */
            AudioBuffer::Result dequeue(uint8_t* buf, const size_t len) {
                auto nb_channels = m_buffers.size();
                for (size_t i = 0; i < nb_channels; i++) {
                    auto len_per_ch = len / nb_channels;
                    auto offset = i * (len_per_ch);
                    if (!m_buffers[i]->read(&buf[offset], len_per_ch)) {
                        return AudioBuffer::Result::OUT_OF_RANGE;
                    }
                    if (!m_buffers[i]->seek(len_per_ch)) {
                        return AudioBuffer::Result::ERR;
                    }
                }

                return AudioBuffer::Result::OK;
            }

            bool seek(const size_t byte_size) {
                for (auto&& buffer : m_buffers) {
                    if (!buffer->seek2(byte_size)) {
                        // [TODO] maybe we should add some rollback for this?
                        return false;
                    }
                }
                return true;
            }

            bool write_ready() const {
                if (!m_back_buffer) {
                    return true;
                }
                if (m_buffers.empty()) {
                    return false;
                }
                auto wbuf_size = m_back_buffer->prop().data_size / m_back_buffer->prop().channels;
                auto wpts = m_back_buffer->prop().pts;
                return this->m_buffers[0]->within_range(wbuf_size, wpts);
            }

            core::Rational cur_pts() const {
                if (m_buffers.empty()) {
                    return core::Rational(-1, 1);
                }
                return m_buffers[0]->buf_pts();
            }

          private:
            std::vector<AudioBuffer::Data> to_buffers(const buffer::AVBufferData& buf_data) const {
                std::vector<AudioBuffer::Data> res_data;
                auto nb_channels = buf_data.prop().channels;
                for (int i = 0; i < nb_channels; i++) {
                    AudioBuffer::Data data;
                    data.len = buf_data.prop().data_size / nb_channels;
                    data.buf = buf_data.prop().audio_data[i];
                    res_data.push_back(std::move(data));
                }
                return res_data;
            }

          private:
            core::owned_ptr<buffer::AVBufferData> m_back_buffer = nullptr;
            std::vector<core::owned_ptr<buffer::AudioRingBuffer2>> m_buffers;
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

            core::owned_ptr<AudioBuffer> abuffer;

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
                    .abuffer = make_owned<AudioBuffer>(encode_audio_spec, audio_max_queue_size),
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
                if (!ctx.state->get_video_decode_ready() || !encode_ctx.abuffer->write_ready()) {
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
                                if (ctx.state->m_encode_conf.video_codec !=
                                    core::EncodeCodec::NONE) {
                                    const auto comp_layer_uuid =
                                        decode_res.layer_uuid + std::to_string(0);
                                    encode_ctx.buffer->vq->enqueue(comp_layer_uuid,
                                                                   std::move(decode_res.buffer));
                                }
                                break;
                            }
                            case buffer::AVBufferType::AUDIO: {
                                if (ctx.state->m_encode_conf.audio_codec !=
                                    core::EncodeCodec::NONE) {
                                    const auto comp_layer_uuid =
                                        decode_res.layer_uuid + std::to_string(0);
                                    auto pts = decode_res.buffer->prop().pts;
                                    encode_ctx.abuffer->enqueue(std::move(decode_res.buffer));
                                    AKLOG_INFO("AudioBuffer Enqueued, pts: {}, id: {}",
                                               pts.to_decimal(), comp_layer_uuid);
                                }
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
                if (result == AudioBuffer::Result::OUT_OF_RANGE) {
                    auto before_pts = encode_ctx.abuffer->cur_pts();
                    encode_ctx.abuffer->seek(queue_data.buf_size);
                    AKLOG_ERROR("AudioBuffer Seeked {} => {}", before_pts.to_decimal(),
                                encode_ctx.abuffer->cur_pts().to_decimal());
                    continue;
                } else if (result != AudioBuffer::Result::OK) {
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

            for (; can_produce(encode_ctx); update_encode_context(encode_ctx)) {
                ctx.queue->wait_for_not_full();

                // eval
                auto frame_ctx = pull_frame_context(borrowed_ptr(eval), encode_ctx);

                // decode
                if (!exec_decode(ctx, encode_ctx)) {
                    break;
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
                    //     save_pcm(dataset.buffer.get() + dataset.buf_size / 2, dataset.buf_size /
                    //     2,
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
