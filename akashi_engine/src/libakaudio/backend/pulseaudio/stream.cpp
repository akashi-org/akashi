#include "./stream.h"
#include "./callback.h"
#include "./context.h"
#include "./util.h"
#include "./etc.h"

#include <libakcore/audio.h>
#include <libakcore/logger.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        AudioStream::AudioStream(core::borrowed_ptr<PulseAudioContext> audio_ctx,
                                 pa_threaded_mainloop* mainloop, pa_context* context)
            : m_audio_ctx(audio_ctx) {
            m_mainloop = mainloop;
            m_context = context;
        };

        void AudioStream::init(void) {
            pa_sample_spec sample_specifications;
            auto audio_spec = m_audio_ctx->audio_spec();
            sample_specifications.format = to_pl_sample_format(audio_spec.format).value;
            sample_specifications.rate = audio_spec.sample_rate;
            sample_specifications.channels = audio_spec.channels;

            pa_channel_map map;
            pa_channel_map_init_stereo(&map);

            m_stream =
                pa_stream_new(m_context, AudioStream::STREAM_NAME, &sample_specifications, &map);
            pa_stream_set_state_callback(m_stream, AudioStream::stream_state_cb, m_mainloop);
            pa_stream_set_write_callback(m_stream, stream_write_cb, m_audio_ctx->cb_ctx());

            pa_buffer_attr buffer_attr;
            // buffer_attr.maxlength = (uint32_t)-1;
            buffer_attr.maxlength = MAX_AUDIO_BUFFER_SIZE;
            buffer_attr.tlength = (uint32_t)-1;
            // [TODO] maybe we need to set this value to zero
            // https://freedesktop.org/software/pulseaudio/doxygen/streams.html#sync_streams
            buffer_attr.prebuf = (uint32_t)-1;
            buffer_attr.minreq = (uint32_t)-1;

            pa_stream_flags_t stream_flags;
            // clang-format off
            stream_flags = static_cast<pa_stream_flags_t>(
                PA_STREAM_START_CORKED 
                | PA_STREAM_INTERPOLATE_TIMING
                | PA_STREAM_NOT_MONOTONIC
                | PA_STREAM_AUTO_TIMING_UPDATE
                | PA_STREAM_ADJUST_LATENCY
            );
            // clang-format on

            pa_stream_connect_playback(m_stream, nullptr, &buffer_attr, stream_flags, nullptr,
                                       nullptr);

            for (;;) {
                pa_stream_state_t stream_state = pa_stream_get_state(m_stream);
                assert(PA_STREAM_IS_GOOD(stream_state));
                if (stream_state == PA_STREAM_READY)
                    break;
                pa_threaded_mainloop_wait(m_mainloop);
            }
        }

        void AudioStream::flush(void) {
            {
                MainloopLockGuard main_lk(m_mainloop);

                PAOpContext<int> op_ctx;
                op_ctx.mainloop = m_mainloop;
                pa_operation* op = pa_stream_flush(
                    m_stream,
                    [](pa_stream*, int success, void* userdata) {
                        auto i_op_ctx = reinterpret_cast<PAOpContext<int>*>(userdata);
                        assert(i_op_ctx->mainloop);
                        i_op_ctx->data = success;
                        i_op_ctx->executed = true;
                        pa_threaded_mainloop_signal(i_op_ctx->mainloop, 0);
                    },
                    &op_ctx);
                while (!op_ctx.executed) {
                    pa_threaded_mainloop_wait(m_mainloop);
                }
                pa_operation_unref(op);
                if (!op_ctx.data) {
                    AKLOG_ERROR("AudioStream::flush() failed: {}", m_audio_ctx->get_pa_error());
                }
            }
            return;
        };

        void AudioStream::cork(void) {
            pa_stream_cork(m_stream, 1, AudioStream::stream_success_cb, nullptr);
        }

        void AudioStream::uncork(void) {
            pa_stream_cork(m_stream, 0, AudioStream::stream_success_cb, nullptr);
        }

        void AudioStream::drain(void) {
            {
                MainloopLockGuard main_lk(m_mainloop);

                PAOpContext<int> op_ctx;
                op_ctx.mainloop = m_mainloop;
                pa_operation* op = pa_stream_drain(
                    m_stream,
                    [](pa_stream*, int success, void* userdata) {
                        auto i_op_ctx = reinterpret_cast<PAOpContext<int>*>(userdata);
                        assert(i_op_ctx->mainloop);
                        i_op_ctx->data = success;
                        i_op_ctx->executed = true;
                        pa_threaded_mainloop_signal(i_op_ctx->mainloop, 0);
                    },
                    &op_ctx);
                while (!op_ctx.executed) {
                    pa_threaded_mainloop_wait(m_mainloop);
                }
                pa_operation_unref(op);
                if (!op_ctx.data) {
                    AKLOG_ERROR("AudioStream::drain() failed: {}", m_audio_ctx->get_pa_error());
                }
            }
            return;
        };

        void AudioStream::destroy(void) {
            pa_stream_disconnect(m_stream);
            pa_stream_unref(m_stream);
        };

        void AudioStream::stream_state_cb(pa_stream*, void* mainloop) {
            pa_threaded_mainloop_signal((pa_threaded_mainloop*)mainloop, 0);
        };

        void AudioStream::stream_success_cb(pa_stream*, int, void*) { return; };

    }
}
