#pragma once

#include <GL/gl.h>

namespace akashi {
    namespace graphics {

        struct GLRenderContext;
        struct GLTextureData;

        void free_texture(const GLRenderContext& ctx, GLTextureData& tex);

        void create_texture(const GLRenderContext& ctx, GLTextureData& tex);

        void use_texture(const GLRenderContext& ctx, const GLTextureData& tex, GLint tex_loc);

    }
}
