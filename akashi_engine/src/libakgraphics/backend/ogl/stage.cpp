#include "./stage.h"

#include "./core/glc.h"
#include "./render_context.h"
#include "./fbo.h"
#include "./camera.h"

#include "../../item.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/error.h>

namespace akashi {
    namespace graphics {

        bool Stage::create(const OGLRenderContext& ctx) {
            this->init_gl();
            return true;
        }

        bool Stage::destroy(const OGLRenderContext& ctx) {}

        bool Stage::render(OGLRenderContext& ctx, const RenderParams& params,
                           const core::FrameContext& frame_ctx) {
            // render layers to fbo
            CHECK_AK_ERROR2(this->render_layers(ctx, frame_ctx));

            // render fbo to the provided framebuffer
            this->init_renderer({params.default_fb, params.screen_width, params.screen_height});
            ctx.fbo().render(ctx.camera()->vp_mat());

            return true;
        }

        bool Stage::encode_render(const OGLRenderContext& ctx,
                                  const core::FrameContext& frame_ctx) {
            return false;
        }

        void Stage::init_gl() {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            // If enabled, we can change the point size in shader
            // glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

            glEnable(GL_MULTISAMPLE);
        }

        void Stage::init_renderer(const FBInfo& info) {
            glBindFramebuffer(GL_FRAMEBUFFER, info.fbo);
            glViewport(0.0, 0.0, info.width, info.height);
            glClearColor(0.5, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        bool Stage::render_layers(OGLRenderContext& ctx, const core::FrameContext& frame_ctx) {
            if (!ctx.fbo().initilized()) {
                AKLOG_WARNN("FBO is not yet initialized");
                return false;
            }

            if (frame_ctx.layer_ctxs.size() == 0 || frame_ctx.layer_ctxs.empty()) {
                AKLOG_WARNN("Layer length is 0");
                return false;
            }

            // activate fbo
            this->init_renderer(ctx.fbo().info());

            ctx.camera()->update(ctx.fbo().info());

            return true;
        }

    }
}
