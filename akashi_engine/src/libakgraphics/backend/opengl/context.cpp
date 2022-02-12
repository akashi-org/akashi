#include "./context.h"
#include "../../item.h"

#include "./osc/osc_root.h"

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

        void OGLGraphicsContext::render(const RenderParams& params,
                                        const core::FrameContext& frame_ctx) {
            if (!m_render_ctx->fbo().initilized()) {
                m_render_ctx->load_fbo();
            }
            m_stage->render(*m_render_ctx, params, frame_ctx);
        }

        void OGLGraphicsContext::encode_render(EncodeRenderParams& params,
                                               const core::FrameContext& frame_ctx) {
            if (!m_render_ctx->fbo().initilized()) {
                m_render_ctx->load_fbo();
            }

            if (m_stage) {
                m_stage->encode_render(*m_render_ctx, frame_ctx);
                params.width = m_render_ctx->fbo().info().width;
                params.height = m_render_ctx->fbo().info().height;

                GLuint dst_fbo;
                m_render_ctx->fbo().dst_fbo(&dst_fbo);

                glBindFramebuffer(GL_FRAMEBUFFER, dst_fbo);

                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                // glReadBuffer(GL_COLOR_ATTACHMENT0);
                glReadPixels(0, 0, params.width, params.height, GL_RGB, GL_UNSIGNED_BYTE,
                             params.buffer);

                glPixelStorei(GL_PACK_ALIGNMENT, 4); // reset to the initial value;

                // another way without glReadPixels
                // OGLTexture fbo_tex;
                // m_render_ctx->fbo().texture(fbo_tex);
                // glBindTexture(GL_TEXTURE_2D, fbo_tex.buffer);
                // glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, params.buffer);

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

        OGLOSCContext::OGLOSCContext(core::borrowed_ptr<state::AKState> state)
            : OSCContext(state), m_state(state){};

        OGLOSCContext::~OGLOSCContext(){};

        bool OGLOSCContext::load_api(OSCEventCallback evt_cb, const RenderParams& params,
                                     const GetProcAddress& get_proc_address,
                                     const EGLGetProcAddress& egl_get_proc_address) {
            // [TODO] we need to resolve a conflict with OGLGraphicsContext::load_api()
            // if (!gladLoadGLLoader((GLADloadproc)get_proc_address.func)) {
            //     AKLOG_ERRORN("Failed to initialize OpenGL context");
            //     return false;
            // }
            // if (!load_egl_functions(egl_get_proc_address)) {
            //     AKLOG_ERRORN("Failed to initialize EGL context");
            //     return false;
            // }

            // init_gl

            // glEnable(GL_DEPTH_TEST);
            // glDepthFunc(GL_LEQUAL);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            m_root = core::make_owned<OSCRoot>(evt_cb, params, m_state);

            return true;
        }

        void OGLOSCContext::render(const RenderParams& params) {
            if (!m_root) {
                AKLOG_ERRORN("m_root is null");
                return;
            }

            if (!m_root->update(params)) {
                AKLOG_DEBUGN("No update needed");
                return;
            }

            m_root->render(params);
        }

        void OGLOSCContext::resize(const RenderParams& params) {
            if (!m_root) {
                AKLOG_ERRORN("m_root is null");
                return;
            }
            m_root->resize(params);
        }

        bool OGLOSCContext::emit_mouse_event(const OSCMouseEvent& event) {
            if (m_root) {
                return m_root->on_mouse_event(event);
            }
            return false;
        }

        bool OGLOSCContext::emit_time_event(const OSCTimeEvent& event) {
            if (m_root) {
                return m_root->on_time_event(event);
            }
            return false;
        }

    }
}
