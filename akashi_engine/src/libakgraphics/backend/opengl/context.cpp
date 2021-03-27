#include "./context.h"
#include "../../item.h"

#include "./gl.h"
#include "./framebuffer.h"
#include "./core/loader.h"
#include "./objects/quad/quad.h"
#include "./objects/quad/video_quad.h"

#include "./render.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>

#include <mutex>
#include <algorithm>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        GLGraphicsContext::GLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                             core::borrowed_ptr<buffer::AVBuffer> buffer)
            : GraphicsContext(state, buffer), m_state(state), m_buffer(buffer) {
            m_render_ctx = make_owned<GLRenderContext>();
        };

        GLGraphicsContext::~GLGraphicsContext() {
            if (m_fbo_pass) {
                m_fbo_pass->destroy(*m_render_ctx);
                m_fbo_pass = nullptr;
            }
        };

        bool GLGraphicsContext::load_api(const GetProcAddress& get_proc_address,
                                         const EGLGetProcAddress& egl_get_proc_address) {
            m_render_ctx->egl_get_proc_address = egl_get_proc_address;
            load_egl_functions(egl_get_proc_address, *m_render_ctx);

            if (load_gl_getString(get_proc_address, *m_render_ctx) != ErrorType::OK) {
                return false;
            }

            CHECK_AK_ERROR2(parse_gl_version(*m_render_ctx));
            CHECK_AK_ERROR2(parse_shader(*m_render_ctx));
            CHECK_AK_ERROR2(parse_gl_extensions(get_proc_address, *m_render_ctx));

            if (load_gl_functions(get_proc_address, *m_render_ctx) != ErrorType::OK) {
                return false;
            }

            // [XXX] make sure to create fbo pass before load_fbo
            m_fbo_pass = new QuadPass;
            CHECK_AK_ERROR2(m_fbo_pass->create(*m_render_ctx));

            m_render_ctx->render_scene = new RenderScene;
            m_render_ctx->render_scene->create(*m_render_ctx);

            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_render_ctx->default_font_path = m_state->m_prop.default_font_path;
            }

            return true;
        }

        bool GLGraphicsContext::load_fbo(const core::RenderProfile& render_prof, bool flip_y) {
            m_render_ctx->fbo = new FramebufferObject;

            int video_width = 0;
            int video_height = 0;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                video_width = m_state->m_prop.video_width;
                video_height = m_state->m_prop.video_height;
            }

            CHECK_AK_ERROR2(m_render_ctx->fbo->create(*m_render_ctx, m_fbo_pass, video_width,
                                                      video_height, flip_y));

            return true;
        }

        void GLGraphicsContext::render(const RenderParams& params,
                                       const core::FrameContext& frame_ctx) {
            if (m_render_ctx->render_scene) {
                m_render_ctx->render_scene->render(borrowed_ptr(this), *m_render_ctx, params,
                                                   frame_ctx);
            }
        }

        void GLGraphicsContext::encode_render(EncodeRenderParams& params,
                                              const core::FrameContext& frame_ctx) {
            if (m_render_ctx->render_scene) {
                m_render_ctx->render_scene->encode_render(borrowed_ptr(this), *m_render_ctx,
                                                          frame_ctx);
                params.width = m_render_ctx->fbo->get_prop().width;
                params.height = m_render_ctx->fbo->get_prop().height;

                GET_GLFUNC((*m_render_ctx), glBindFramebuffer)
                (GL_FRAMEBUFFER, m_render_ctx->fbo->get_prop().fbo);

                GET_GLFUNC((*m_render_ctx), glPixelStorei)(GL_PACK_ALIGNMENT, 1);
                // glReadBuffer(GL_COLOR_ATTACHMENT0);
                GET_GLFUNC((*m_render_ctx), glReadPixels)
                (0, 0, params.width, params.height, GL_RGB, GL_UNSIGNED_BYTE, params.buffer);

                GET_GLFUNC((*m_render_ctx), glPixelStorei)
                (GL_PACK_ALIGNMENT, 4); // reset to the initial value;

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

        size_t GLGraphicsContext::loop_cnt() {
            size_t loop_cnt = 0;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                loop_cnt = m_state->m_prop.loop_cnt;
            }
            return loop_cnt;
        }

        bool GLGraphicsContext::shader_reload() {
            bool shader_reload = false;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                shader_reload = m_state->m_prop.shader_reload;
            }
            return shader_reload;
        }

        void GLGraphicsContext::set_shader_reload(bool reloaded) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.shader_reload = reloaded;
            }
        }

        std::vector<const char*> GLGraphicsContext::updated_shader_paths(void) {
            std::vector<const char*> paths;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                paths = m_state->m_prop.updated_shader_paths;
            }
            return paths;
        }

        std::array<int, 2> GLGraphicsContext::resolution() {
            std::array<int, 2> res{0, 0};
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                res[0] = m_state->m_prop.video_width;
                res[1] = m_state->m_prop.video_height;
            }
            return res;
        }

        std::unique_ptr<buffer::AVBufferData>
        GLGraphicsContext::dequeue(std::string layer_uuid, const core::Rational& pts) {
            return m_buffer->vq->dequeue(layer_uuid, pts);
        }

    }
}
