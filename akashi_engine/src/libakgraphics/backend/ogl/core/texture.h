#pragma once

#include "./glc.h"

#include <glm/glm.hpp>

namespace akashi {
    namespace graphics {

        struct OGLTexture {
            int index = 0;
            GLuint buffer;
            void* image = nullptr;
            void* surface = nullptr;
            int width;  // includes stride
            int height; // includes stdide
            int effective_width;
            int effective_height;
            GLenum format = GL_RGBA;
            GLenum internal_format = GL_RGBA;
            GLenum target = GL_TEXTURE_2D;
            int8_t reversed =
                0; // if 1, tex format is GL_RG8, but the order of r and g are reversed
        };

        void free_ogl_texture(OGLTexture& tex);

        void use_ogl_texture(const OGLTexture& tex, GLint tex_loc);

    }
}
