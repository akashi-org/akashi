#pragma once

#include <libakcore/memory.h>
#include <libakcore/element.h>

#include <mutex>
#include <deque>
#include <vector>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace state {
        class AKState;
    }
    namespace player {

        class EvalBuffer final {
          public:
            inline const static core::FrameContext BLANK_FRAME_CTX = {
                core::Rational{-100, 1}, {}, {}};

          public:
            explicit EvalBuffer(core::borrowed_ptr<state::AKState> state);

            virtual ~EvalBuffer();

            void set_render_buf(const core::FrameContext frame_ctx);

            bool fetch_render_buf();

            const core::FrameContext render_buf(void);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            struct {
                core::FrameContext render_buf = BLANK_FRAME_CTX;
                std::mutex mtx;
            } m_synced_render_buf;
        };
    }
}
