#pragma once

#include <libakcore/memory.h>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace state {
        class AKState;
    }
    namespace audio {
        class AKAudio;
    }
    namespace eval {
        class AKEval;
    }
    namespace watch {
        struct WatchEventList;
    }
    namespace player {

        class PlayerEvent;
        class EvalBuffer;

        class HRManager final {
          public:
            explicit HRManager(core::borrowed_ptr<state::AKState> state,
                               core::borrowed_ptr<buffer::AVBuffer> buffer,
                               core::borrowed_ptr<PlayerEvent> event,
                               core::borrowed_ptr<EvalBuffer> eval_buf,
                               core::borrowed_ptr<eval::AKEval> eval);
            virtual ~HRManager();

            void reload(const watch::WatchEventList& event_list);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;
            core::borrowed_ptr<PlayerEvent> m_event;
            core::borrowed_ptr<EvalBuffer> m_eval_buf;
            core::borrowed_ptr<eval::AKEval> m_eval;
        };

    }

}
