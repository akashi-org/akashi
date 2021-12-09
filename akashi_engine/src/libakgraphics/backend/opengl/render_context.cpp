#include "./render_context.h"
#include "./fbo.h"
#include "./camera.h"

#include <libakcore/memory.h>
#include <libakcore/error.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>

#include <libakcore/logger.h>

#include <array>

namespace akashi {
    namespace graphics {

        OGLRenderContext::OGLRenderContext(core::borrowed_ptr<state::AKState> state,
                                           core::borrowed_ptr<buffer::AVBuffer> buffer)
            : m_state(state), m_buffer(buffer) {
            m_fbo = core::make_owned<FBO>();
        }

        OGLRenderContext::~OGLRenderContext() { m_fbo->destroy(); }

        const FBO& OGLRenderContext::fbo() const { return *m_fbo; }

        bool OGLRenderContext::load_fbo() {
            int video_width = 0;
            int video_height = 0;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                video_width = m_state->m_prop.video_width;
                video_height = m_state->m_prop.video_height;
            }

            CHECK_AK_ERROR2(m_fbo->create(video_width, video_height));

            ProjectionState proj_state;
            proj_state.video_width = video_width;
            proj_state.video_height = video_height;

            ViewState view_state;
            view_state.camera = glm::vec3(0, 0, 1);

            m_camera = core::make_owned<Camera>(proj_state, &view_state);

            return true;
        }

        core::borrowed_ptr<Camera> OGLRenderContext::mut_camera() {
            return core::borrowed_ptr(m_camera.get());
        }

        const core::borrowed_ptr<Camera> OGLRenderContext::camera() const {
            return core::borrowed_ptr(m_camera.get());
        }

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

        std::string OGLRenderContext::default_font_path() {
            std::string font_path;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                font_path = m_state->m_prop.default_font_path;
            }
            return font_path;
        }

        std::unique_ptr<buffer::AVBufferData> OGLRenderContext::dequeue(std::string layer_uuid,
                                                                        const core::Rational& pts) {
            return m_buffer->vq->dequeue(layer_uuid, pts);
        }

    }
}
