#pragma once

#include <libakcore/memory.h>
#include <string>

namespace akashi {
    namespace core {
        class Rational;
        class Path;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace state {
        class AKState;
    }
    namespace eval {
        class AKEval;
    }
    namespace player {

        class PlayerEvent;
        class EvalBuffer;

        namespace reload {

            struct ReloadContext {
                core::borrowed_ptr<state::AKState> state;
                core::borrowed_ptr<buffer::AVBuffer> buffer;
                core::borrowed_ptr<PlayerEvent> event;
                core::borrowed_ptr<EvalBuffer> eval_buf;
                core::borrowed_ptr<eval::AKEval> eval;
            };

            void time_update(ReloadContext& rctx, const core::Rational& seek_time);

            bool reload_avbuffer(ReloadContext& rctx, const core::Rational& seek_time,
                                 bool skip_seek = false);

            void exec_global_eval(ReloadContext& rctx);

            bool exec_local_eval(ReloadContext& rctx, const core::Rational& seek_time,
                                 bool skip_seek = false);

            void render_update(ReloadContext& rctx);

        }

    }
}
