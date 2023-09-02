#include "./eval_buffer.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>
#include <libakeval/item.h>

#include <mutex>
#include <deque>

using namespace akashi::core;

namespace akashi {
    namespace player {

        static bool local_eval(core::FrameContext* frame_ctx,
                               core::borrowed_ptr<state::AKState> state) {
            *frame_ctx = EvalBuffer::BLANK_FRAME_CTX;
            Rational fps;
            Rational start_time;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                fps = state->m_prop.fps;
                start_time = state->m_prop.current_time;
            }

            // auto is_init_pts = start_time.num() == 0 ? true : false;
            // if (!is_init_pts) {
            //     start_time += (Rational(1, 1) / fps);
            // }

            // [TODO] Perhaps we need to wait eval_gctx_ready?
            {
                std::lock_guard<std::mutex> lock(state->m_eval_gctx_mtx);
                auto gctx = reinterpret_cast<eval::GlobalContext*>(state->m_eval_gctx);
                if (gctx) {
                    *frame_ctx = gctx->local_eval(
                        core::borrowed_ptr(gctx),
                        {.play_time = start_time, .fps = static_cast<long>(fps.to_decimal())});
                    return true;
                } else {
                    return false;
                }
            }
        }

        EvalBuffer::EvalBuffer(core::borrowed_ptr<state::AKState> state) : m_state(state) {}

        EvalBuffer::~EvalBuffer() {}

        void EvalBuffer::set_render_buf(const core::FrameContext frame_ctx) {
            {
                std::lock_guard<std::mutex> lock(m_synced_render_buf.mtx);
                m_synced_render_buf.render_buf = frame_ctx;
            }
        }

        bool EvalBuffer::fetch_render_buf() {
            core::FrameContext frame_ctx;
            auto fetch_result = local_eval(&frame_ctx, m_state);
            if (fetch_result) {
                this->set_render_buf(frame_ctx);
            }
            return fetch_result;
        }

        const core::FrameContext EvalBuffer::render_buf(void) {
            std::lock_guard<std::mutex> lock(m_synced_render_buf.mtx);
            return m_synced_render_buf.render_buf;
        }

    }
}
