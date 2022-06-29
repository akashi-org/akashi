#pragma once

#include "./hwaccel.h"
#include "../core/glc.h"

namespace akashi {
    namespace buffer {
        class HWFrame;
        struct HWFrameInfo;
        struct VAAPIEGLContext;
    }
    namespace graphics {

        class OGLRenderContext;

        class VAAPIHWEncodeFBO final : public HWEncodeFBO {
          public:
            struct Context;

          public:
            explicit VAAPIHWEncodeFBO(OGLRenderContext& render_ctx);

            virtual ~VAAPIHWEncodeFBO();

            virtual void render(OGLRenderContext& render_ctx, GLuint rgba_tex,
                                core::borrowed_ptr<buffer::HWFrame> hwframe) override;

          private:
            bool load_fbo(OGLRenderContext& render_ctx);

            bool load_pass(OGLRenderContext& render_ctx);

            void create_mesh(OGLRenderContext& render_ctx);

            bool load_external_textures(buffer::VAAPIEGLContext* egl_ctx,
                                        const buffer::HWFrameInfo& hw_info);

          private:
            VAAPIHWEncodeFBO::Context* m_ctx;
        };

    }
}
