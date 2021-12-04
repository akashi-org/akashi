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
            inline const static core::FrameContext BLANK_FRAME_CTX = {core::Rational{-100, 1}, {}};

          public:
            constexpr static size_t BUFFER_MAX_LENGTH = 300;
            constexpr static size_t BUFFER_READY_LENGTH = 50;

          public:
            explicit EvalBuffer(core::borrowed_ptr<state::AKState> state);

            virtual ~EvalBuffer();

            void push(const std::vector<core::FrameContext>& frame_ctxs);

            void pop(void);

            void clear(void);

            void pull(bool force = false);

            const core::FrameContext& front(void);

            const core::FrameContext& back(void);

            bool seek(const core::Rational& seek_time);

            void set_render_buf(const core::FrameContext frame_ctx);

            const core::FrameContext& render_buf(void);

            size_t size(void);

            bool empty(void);

            bool is_hungry(void);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            struct {
                std::deque<core::FrameContext> buf;
                std::mutex mtx;
            } m_synced_buf;
            struct {
                core::FrameContext render_buf = BLANK_FRAME_CTX;
                std::mutex mtx;
            } m_synced_render_buf;
        };
    }
}
