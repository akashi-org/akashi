#include "./glcontext.h"

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>

#include <array>

namespace akashi {
    namespace graphics {

        OGLRenderContext::OGLRenderContext(core::borrowed_ptr<state::AKState> state,
                                           core::borrowed_ptr<buffer::AVBuffer> buffer)
            : m_state(state), m_buffer(buffer) {}

        OGLRenderContext::~OGLRenderContext() {}

        size_t OGLRenderContext::loop_cnt() { return m_state->m_atomic_state.play_loop_cnt; }

        core::Rational OGLRenderContext::fps() {
            core::Rational fps;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                fps = m_state->m_prop.fps;
            }
            return fps;
        }

        std::array<int, 2> OGLRenderContext::resolution() {
            std::array<int, 2> res{0, 0};
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                res[0] = m_state->m_prop.video_width;
                res[1] = m_state->m_prop.video_height;
            }
            return res;
        }

        std::unique_ptr<buffer::AVBufferData> OGLRenderContext::dequeue(std::string layer_uuid,
                                                                        const core::Rational& pts) {
            return m_buffer->vq->dequeue(layer_uuid, pts);
        }

    }
}
