#include "./context.h"
#include "../../item.h"

#include "./core/glc.h"
#include "./core/eglc.h"
#include "./render_context.h"
#include "./stage.h"
#include "./actors/actor.h"
#include "./fbo.h"

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
            if (!load_egl_functions(egl_get_proc_address)) {
                AKLOG_ERRORN("Failed to initialize EGL context");
                return false;
            }
            return m_stage->create(*m_render_ctx);
        }

        // [TODO] we need to deprecate the API
        // Perhaps, makeCurrent() is not called in this path
        bool OGLGraphicsContext::load_fbo(const core::RenderProfile& /* render_prof */,
                                          bool /* flip_y */) {
            // return m_render_ctx->load_fbo();
            return true;
        }

        void OGLGraphicsContext::render(const RenderParams& params,
                                        const core::FrameContext& frame_ctx) {
            if (!m_render_ctx->fbo().initilized()) {
                m_render_ctx->load_fbo();
            }
            m_stage->render(*m_render_ctx, params, frame_ctx);
        }

        void OGLGraphicsContext::encode_render(EncodeRenderParams& params,
                                               const core::FrameContext& frame_ctx) {
            if (m_stage) {
                m_stage->encode_render(*m_render_ctx, frame_ctx);
                params.width = m_render_ctx->fbo().info().width;
                params.height = m_render_ctx->fbo().info().height;

                glBindFramebuffer(GL_FRAMEBUFFER, m_render_ctx->fbo().info().fbo);

                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                // glReadBuffer(GL_COLOR_ATTACHMENT0);
                glReadPixels(0, 0, params.width, params.height, GL_RGB, GL_UNSIGNED_BYTE,
                             params.buffer);

                glPixelStorei(GL_PACK_ALIGNMENT, 4); // reset to the initial value;

                // rgbrgbrgbrgb... [line0]
                // rgbrgbrgbrgb... [line1]
                // ...
                // rgbrgbrgbrgb... [line height/2]
                // ...
                // ... [line height - 1]
                // where row length = width * 3

                // [TODO] maybe we should render upside down in advance?
                // flip the buffer vertically
                for (int line = 0; line < params.height / 2; line++) {
                    std::swap_ranges(params.buffer + 3 * params.width * line,
                                     params.buffer + 3 * params.width * (line + 1),
                                     params.buffer + 3 * params.width * (params.height - line - 1));
                }
            }
        }

    }
}
