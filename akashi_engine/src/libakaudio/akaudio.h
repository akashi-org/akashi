#pragma once

#include <libakcore/memory.h>

#include <atomic>
#include <vector>
#include <memory>

namespace akashi {
    namespace core {
        class Rational;
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
        class AudioContext;
        class AKAudio {
          public:
            explicit AKAudio(core::borrowed_ptr<state::AKState> state,
                             core::borrowed_ptr<buffer::AVBuffer> buffer,
                             core::borrowed_ptr<event::AKEvent> event);
            virtual ~AKAudio();

            void destroy(void);

            void play(void);

            void pause(void);

            void stop(void);

            core::Rational current_time(void) const;

          private:
            core::owned_ptr<AudioContext> m_audio_ctx;
        };

    }

}
