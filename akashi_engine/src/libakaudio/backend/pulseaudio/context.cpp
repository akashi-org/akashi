#include "./context.h"
#include "./callback.h"
#include "./stream.h"
#include "./util.h"
#include "./etc.h"

#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        PulseAudioContext::PulseAudioContext(core::borrowed_ptr<state::AKState> state,
                                             core::borrowed_ptr<buffer::AVBuffer> buffer,
                                             core::borrowed_ptr<event::AKEvent> event)
            : AudioContext(state, buffer, event), m_state(state), m_buffer(buffer) {
            m_mainloop = pa_threaded_mainloop_new();
            assert(m_mainloop);
            {
                MainloopLockGuard main_lk(m_mainloop);
                m_context = pa_context_new(pa_threaded_mainloop_get_api(m_mainloop),
                                           PulseAudioContext::PA_APPLICATION_NAME);
                assert(m_context);

                pa_context_set_state_callback(m_context, PulseAudioContext::context_state_cb,
                                              m_mainloop);

                m_cb_ctx = new CallbackContext(core::borrowed_ptr(this), state, buffer, event);
                m_stream = new AudioStream(core::borrowed_ptr(this), m_mainloop, m_context);

                pa_threaded_mainloop_start(m_mainloop);

                pa_context_connect(m_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

                for (;;) {
                    pa_context_state_t context_state = pa_context_get_state(m_context);
                    assert(PA_CONTEXT_IS_GOOD(context_state));
                    if (context_state == PA_CONTEXT_READY)
                        break;
                    pa_threaded_mainloop_wait(m_mainloop);
                }

                m_stream->init();

                for (;;) {
                    pa_stream_state_t stream_state = pa_stream_get_state(m_stream->stream());
                    assert(PA_STREAM_IS_GOOD(stream_state));
                    if (stream_state == PA_STREAM_READY)
                        break;
                    pa_threaded_mainloop_wait(m_mainloop);
                }
            }
        };

        PulseAudioContext::~PulseAudioContext() { this->destroy(); };

        void PulseAudioContext::destroy(void) {
            // [TODO] looks buggy
            // see https://gist.github.com/toroidal-code/8798775
            if (m_stream) {
                this->stop();
                // https://lists.freedesktop.org/archives/pulseaudio-bugs/2010-October/004191.html
                // mozilla seems to abandon a drain
                // m_stream->drain(); // [TODO] timeout issues when executing drain
                m_stream->destroy();
                delete m_stream;
                m_stream = nullptr;
            }

            if (m_context) {
                pa_context_disconnect(m_context);
                pa_context_unref(m_context);
                m_context = nullptr;
            }

            if (m_mainloop) {
                pa_threaded_mainloop_stop(m_mainloop);
                pa_threaded_mainloop_free(m_mainloop);
                m_mainloop = nullptr;
            }

            if (m_cb_ctx) {
                m_cb_ctx->destroy();
                delete m_cb_ctx;
                m_cb_ctx = nullptr;
            }

            AKLOG_INFON("PulseAudioContext::destroy(): Successfully exited");
        }

        void PulseAudioContext::play(void) {
            if (m_state->m_atomic_state.audio_play_state != state::PlayState::PLAYING) {
                m_state->m_atomic_state.audio_play_state.store(state::PlayState::PLAYING);
                m_stream->uncork();
            }
        };

        void PulseAudioContext::pause(void) {
            if (m_state->m_atomic_state.audio_play_state != state::PlayState::PAUSED) {
                m_state->m_atomic_state.audio_play_state.store(state::PlayState::PAUSED);
                m_stream->cork();
                m_stream->flush();
            }
        };

        void PulseAudioContext::stop(void) {
            if (m_state->m_atomic_state.audio_play_state != state::PlayState::STOPPED) {
                m_state->m_atomic_state.audio_play_state.store(state::PlayState::STOPPED);
                m_stream->flush();
            }
        };

        core::Rational PulseAudioContext::current_time(void) const {
            return m_state->m_atomic_state.start_time.load() +
                   this->to_pts(m_state->m_atomic_state.bytes_played.load());
        };

        core::AKAudioSpec PulseAudioContext::audio_spec(void) const {
            return m_state->m_atomic_state.audio_spec.load();
        };

        const char* PulseAudioContext::get_pa_error(void) const {
            return pa_strerror(pa_context_errno(m_context));
        };

        // [TODO] is it time-consuming to call this every time?
        size_t PulseAudioContext::bytes_per_second(void) const {
            pa_sample_spec spec;
            auto audio_spec = this->audio_spec();
            spec.rate = audio_spec.sample_rate;
            spec.format = to_pl_sample_format(audio_spec.format).value;
            spec.channels = audio_spec.channels;
            return pa_bytes_per_second(&spec);
        };

        core::Rational PulseAudioContext::to_pts(size_t bytes) const {
            return Rational(bytes, this->bytes_per_second());
        };

        void PulseAudioContext::context_state_cb(pa_context*, void* mainloop) {
            pa_threaded_mainloop_signal((pa_threaded_mainloop*)mainloop, 0);
        }

    }
}
