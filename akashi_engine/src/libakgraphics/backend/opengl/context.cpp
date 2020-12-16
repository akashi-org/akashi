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

        GLGraphicsContext::~GLGraphicsContext(){};

        bool GLGraphicsContext::load_api(const GetProcAddress& get_proc_address) {
            if (load_gl_getString(get_proc_address, *m_render_ctx) != ErrorType::OK) {
                return false;
            }

            CHECK_AK_ERROR2(parse_gl_version(*m_render_ctx));
            CHECK_AK_ERROR2(parse_shader(*m_render_ctx));
            CHECK_AK_ERROR2(parse_gl_extensions(get_proc_address, *m_render_ctx));

            if (load_gl_functions(get_proc_address, *m_render_ctx) != ErrorType::OK) {
                return false;
            }

            m_render_ctx->pass = new QuadPass;
            CHECK_AK_ERROR2(m_render_ctx->pass->create(*m_render_ctx));

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

            CHECK_AK_ERROR2(m_render_ctx->fbo->create(*m_render_ctx, video_width, video_height));

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

        core::Rational GLGraphicsContext::current_time() const { return m_audio->current_time(); }

        std::unique_ptr<buffer::AVBufferData>
        GLGraphicsContext::dequeue(std::string layer_uuid, const core::Rational& pts) {
            return m_buffer->vq->dequeue(layer_uuid, pts);
        }

    }
}
