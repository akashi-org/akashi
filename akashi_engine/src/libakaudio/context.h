#pragma once

#include <libakcore/memory.h>
#include <libakcore/element.h>

namespace akashi {
    namespace core {
        class Rational;
        enum class PlayState;
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
        class AudioContext {
          public:
            explicit AudioContext(core::borrowed_ptr<state::AKState>,
                                  core::borrowed_ptr<buffer::AVBuffer>,
                                  core::borrowed_ptr<event::AKEvent>){};
            virtual ~AudioContext(){};
            virtual void destroy(void) = 0;
            virtual void play(void) = 0;
            virtual void pause(void) = 0;
            virtual void stop(void) = 0;
            virtual core::Rational current_time(void) const = 0;
        };

    }
}
