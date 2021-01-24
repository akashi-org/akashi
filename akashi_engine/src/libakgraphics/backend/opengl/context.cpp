#include "./context.h"
#include "../../item.h"

#include "./gl.h"
#include "./framebuffer.h"
#include "./core/loader.h"
#include "./objects/quad/quad.h"
#include "./objects/quad/video_quad.h"

#include "./render.h"

#include <libakcore/logger.h>
#include <libakaudio/akaudio.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>

#include <mutex>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        GLGraphicsContext::GLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                             core::borrowed_ptr<buffer::AVBuffer> buffer,
                                             core::borrowed_ptr<audio::AKAudio> audio)
            : GraphicsContext(state, buffer, audio), m_state(state), m_buffer(buffer),
              m_audio(audio) {
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

        bool GLGraphicsContext::load_fbo(const core::RenderProfile& render_prof) {
            m_render_ctx->fbo = new FramebufferObject;

            int video_width = 0;
            int video_height = 0;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                video_width = m_state->m_prop.video_width;
                video_height = m_state->m_prop.video_height;
            }

            CHECK_AK_ERROR2(
                m_render_ctx->fbo->create(*m_render_ctx, m_fbo_pass, video_width, video_height));

            return true;
        }

        void GLGraphicsContext::render(const RenderParams& params,
                                       const core::FrameContext& frame_ctx) {
            if (m_render_ctx->render_scene) {
                m_render_ctx->render_scene->render(borrowed_ptr(this), *m_render_ctx, params,
                                                   frame_ctx);
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

        core::Rational GLGraphicsContext::current_time() const { return m_audio->current_time(); }

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
