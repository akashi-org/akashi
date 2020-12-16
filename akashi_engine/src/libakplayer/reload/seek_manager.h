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
    namespace player {

        class PlayerEvent;
        class EvalBuffer;

        class SeekManager final {
          public:
            explicit SeekManager(core::borrowed_ptr<state::AKState> state,
                                 core::borrowed_ptr<buffer::AVBuffer> buffer,
                                 core::borrowed_ptr<PlayerEvent> event,
                                 core::borrowed_ptr<EvalBuffer> eval_buf,
                                 core::borrowed_ptr<eval::AKEval> eval);
            virtual ~SeekManager();

            void seek(const core::Rational& seek_time);

            bool can_seek(const core::Rational& seek_time);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;
            core::borrowed_ptr<PlayerEvent> m_event;
            core::borrowed_ptr<EvalBuffer> m_eval_buf;
            core::borrowed_ptr<eval::AKEval> m_eval;
        };

    }

}
