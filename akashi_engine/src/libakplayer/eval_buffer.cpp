#include "./eval_buffer.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>

#include <mutex>
#include <deque>

using namespace akashi::core;

namespace akashi {
    namespace player {

        EvalBuffer::EvalBuffer(core::borrowed_ptr<state::AKState> state) : m_state(state) {}

        EvalBuffer::~EvalBuffer() {}

        void EvalBuffer::push(const std::vector<core::FrameContext>& frame_ctxs) {
            size_t buf_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                for (const auto& frame_ctx : frame_ctxs) {
                    m_synced_buf.buf.push_front(std::move(frame_ctx));
                }
                buf_size = m_synced_buf.buf.size();
            }

            if (buf_size >= EvalBuffer::BUFFER_READY_LENGTH) {
                m_state->set_evalbuf_dequeue_ready(true);
            }
        }

        void EvalBuffer::pop(void) {
            size_t buf_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                if (m_synced_buf.buf.empty()) {
                    return;
                }
                m_synced_buf.buf.pop_back();
                buf_size = m_synced_buf.buf.size();
            }
            // if (buf_size < EvalBuffer::BUFFER_READY_LENGTH) {
            //     m_state->set_evalbuf_dequeue_ready(false, true);
            // }
            return;
        }

        void EvalBuffer::clear(void) {
            std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
            if (!m_synced_buf.buf.empty()) {
                m_synced_buf.buf.clear();
            }
            m_state->set_evalbuf_dequeue_ready(false, true);
        }

        const core::FrameContext& EvalBuffer::front(void) {
            std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
            if (m_synced_buf.buf.empty()) {
                return EvalBuffer::BLANK_FRAME_CTX;
            } else {
                return m_synced_buf.buf.front();
            }
        }

        const core::FrameContext& EvalBuffer::back(void) {
            std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
            if (m_synced_buf.buf.empty()) {
                return EvalBuffer::BLANK_FRAME_CTX;
            } else {
                return m_synced_buf.buf.back();
            }
        }

        bool EvalBuffer::seek(const core::Rational& seek_time) {
            bool result = false;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                while (!m_synced_buf.buf.empty()) {
                    const auto& current_frame_ctx = m_synced_buf.buf.back();
                    // if found
                    if (seek_time == current_frame_ctx.pts) {
                        result = true;
                        break;
                    }
                    // if not found
                    else {
                        auto old_frame_ctx = std::move(m_synced_buf.buf.back());
                        m_synced_buf.buf.pop_back();
                    }
                }
            }
            this->set_render_buf(result ? this->back() : BLANK_FRAME_CTX);
            return result;
        }

        void EvalBuffer::set_render_buf(const core::FrameContext frame_ctx) {
            {
                std::lock_guard<std::mutex> lock(m_synced_render_buf.mtx);
                m_synced_render_buf.render_buf = frame_ctx;
            }
        }

        const core::FrameContext& EvalBuffer::render_buf(void) {
            std::lock_guard<std::mutex> lock(m_synced_render_buf.mtx);
            return m_synced_render_buf.render_buf;
        }

        size_t EvalBuffer::size(void) {
            size_t size = 0;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                size = m_synced_buf.buf.size();
            }
            return size;
        }

        bool EvalBuffer::empty(void) {
            bool empty = false;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                empty = m_synced_buf.buf.empty();
            }
            return empty;
        }

        bool EvalBuffer::is_hungry(void) { return this->size() < EvalBuffer::BUFFER_READY_LENGTH; }

    }
}
