#include "./context.h"
#include "../../item.h"

#include "./core/glc.h"
#include "./render_context.h"
#include "./stage.h"
#include "./actors/actor.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        OGLGraphicsContext::OGLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                               core::borrowed_ptr<buffer::AVBuffer> buffer)
            : GraphicsContext(state, buffer) {
            m_render_ctx = core::make_owned<OGLRenderContext>(state, buffer);
            m_stage = core::make_owned<Stage>();
        };

        OGLGraphicsContext::~OGLGraphicsContext(){};

        bool OGLGraphicsContext::load_api(const GetProcAddress& get_proc_address,
                                          const EGLGetProcAddress& egl_get_proc_address) {
            if (!gladLoadGLLoader((GLADloadproc)get_proc_address.func)) {
                AKLOG_ERRORN("Failed to initialize OpenGL context");
                return false;
            }

            // [TODO] load egl manually

            return m_stage->create(*m_render_ctx);
        }

        // [TODO] we need to change the API
        bool OGLGraphicsContext::load_fbo(const core::RenderProfile& /* render_prof */,
                                          bool /* flip_y */) {
            return m_render_ctx->load_fbo();
        }

        void OGLGraphicsContext::render(const RenderParams& params,
                                        const core::FrameContext& frame_ctx) {
            m_stage->render(*m_render_ctx, params, frame_ctx);
        }

        void OGLGraphicsContext::encode_render(EncodeRenderParams& params,
                                               const core::FrameContext& frame_ctx) {}

    }
}
