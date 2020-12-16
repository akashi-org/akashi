#include "./texture.h"

#include "../gl.h"

// [TODO] we should not refer to headers in resource dir
#include "../resource/font.h"
#include "../resource/image.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <SDL.h>
#include <string>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        static GLenum get_texture_unit(int texture_index) {
            // [TODO] there might be portablity issues
            return GL_TEXTURE0 + texture_index;
        };

        void free_texture(const GLRenderContext& ctx, GLTextureData& tex) {
            GET_GLFUNC(ctx, glDeleteTextures)(1, &tex.buffer);
            SDL_FreeSurface((SDL_Surface*)tex.surface);
            tex.image = nullptr;
            tex.surface = nullptr;
        }

        void create_texture(const GLRenderContext& ctx, GLTextureData& tex) {
            GET_GLFUNC(ctx, glGenTextures)(1, &tex.buffer);

            GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
            GET_GLFUNC(ctx, glTexImage2D)
            (GL_TEXTURE_2D, 0, tex.internal_format, tex.width, tex.height, 0, tex.format,
             GL_UNSIGNED_BYTE, tex.image);

            // GET_GLFUNC(ctx, glGenerateMipmap)(GL_TEXTURE_2D);

            // [XXX] make sure to explicity setup when not using mimap
            GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, 0);
        }

        void use_texture(const GLRenderContext& ctx, const GLTextureData& tex, GLint tex_loc) {
            GET_GLFUNC(ctx, glActiveTexture)(get_texture_unit(tex.index));
            GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
            GET_GLFUNC(ctx, glUniform1i)(tex_loc, tex.index);
        }

    }
}
