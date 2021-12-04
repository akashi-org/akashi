#pragma once

#include "../../context.h"

#include <atomic>
#include <vector>

typedef struct pa_stream pa_stream;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;
typedef struct pa_context pa_context;

namespace akashi {
    namespace core {
        class Rational;
        struct AtomProfile;
        enum class PlayState;
        struct AKAudioSpec;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace event {
        class AKEvent;
    }
    namespace state {
        class AKState;
    }
    namespace audio {

        class CallbackContext;
        class AudioStream;
        class PulseAudioContext : public AudioContext {
          public:
            static constexpr const char PA_APPLICATION_NAME[] = "akashi-pa-app";

          public:
            explicit PulseAudioContext(core::borrowed_ptr<state::AKState> state,
                                       core::borrowed_ptr<buffer::AVBuffer> buffer,
                                       core::borrowed_ptr<event::AKEvent> event);
            virtual ~PulseAudioContext();

            void destroy(void) override;

            void play(void) override;

            void pause(void) override;

            void stop(void) override;

            core::Rational current_time(void) const override;

            core::AKAudioSpec audio_spec(void) const;

            const char* get_pa_error(void) const;

            size_t bytes_per_second(void) const;

            core::Rational to_pts(size_t bytes) const;

            CallbackContext* cb_ctx(void) const { return m_cb_ctx; }

          private:
            static void context_state_cb(pa_context* context, void* mainloop);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;

            CallbackContext* m_cb_ctx = nullptr;

            AudioStream* m_stream = nullptr;

            pa_threaded_mainloop* m_mainloop = nullptr;
            pa_context* m_context = nullptr;
        };

    }
}
