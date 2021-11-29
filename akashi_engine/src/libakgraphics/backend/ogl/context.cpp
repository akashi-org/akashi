#include "./context.h"
#include "../../item.h"

#include "./glc.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        OGLGraphicsContext::OGLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                               core::borrowed_ptr<buffer::AVBuffer> buffer)
            : GraphicsContext(state, buffer), m_state(state), m_buffer(buffer){};

        OGLGraphicsContext::~OGLGraphicsContext(){};

        bool OGLGraphicsContext::load_api(const GetProcAddress& get_proc_address,
                                          const EGLGetProcAddress& egl_get_proc_address) {
            if (!gladLoadGLLoader((GLADloadproc)get_proc_address.func)) {
                AKLOG_ERRORN("Failed to initialize OpenGL context");
                return false;
            }

            // AKLOG_DEBUG("GL_VERSION: {}", glGetString(GL_VERSION));
            // AKLOG_DEBUG("GL_VENDOR: {}", glGetString(GL_VENDOR));
            // AKLOG_DEBUG("GL_RENDERER: {}", glGetString(GL_RENDERER));

            return true;
        }

        bool OGLGraphicsContext::load_fbo(const core::RenderProfile& render_prof, bool flip_y) {
            return true;
        }

        void OGLGraphicsContext::render(const RenderParams& params,
                                        const core::FrameContext& frame_ctx) {}

        void OGLGraphicsContext::encode_render(EncodeRenderParams& params,
                                               const core::FrameContext& frame_ctx) {}

    }
}
