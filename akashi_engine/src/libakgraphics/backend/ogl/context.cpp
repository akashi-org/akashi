#include "./context.h"
#include "../../item.h"

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
