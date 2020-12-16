#pragma once

#include "./objects/quad/quad.h"

#include <libakcore/error.h>

namespace akashi {
    namespace graphics {

        struct GLRenderContext;

        class FramebufferObject {
            struct Property {
                GLuint fbo;
                QuadObject quad;
                int width;
                int height;
            };

          public:
            const static GLuint FBO_TEX_UNIT = 0;

          public:
            explicit FramebufferObject(void) = default;
            ~FramebufferObject(void) = default;

            bool create(const GLRenderContext& ctx, int fbo_width, int fbo_height);
            bool render(const GLRenderContext& ctx) const;
            bool destroy(const GLRenderContext& ctx);
            const FramebufferObject::Property& get_prop(void) const;

          private:
            bool load_fbo(const GLRenderContext& ctx, FramebufferObject::Property& prop,
                          GLTextureData& tex, int fbo_width, int fbo_height) const;
            bool load_fbo_texture(const GLRenderContext& ctx, GLTextureData& tex,
                                  const FramebufferObject::Property& prop) const;

          private:
            Property m_prop;
        };

    }
}
