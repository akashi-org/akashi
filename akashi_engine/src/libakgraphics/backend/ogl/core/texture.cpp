#include "./texture.h"

#include <SDL.h>

namespace akashi {
    namespace graphics {

        namespace priv {

            static GLenum get_texture_unit(int texture_index) {
                // [TODO] there might be portablity issues
                return GL_TEXTURE0 + texture_index;
            };
        }

        void free_ogl_texture(OGLTexture& tex) {
            glDeleteTextures(1, &tex.buffer);
            SDL_FreeSurface((SDL_Surface*)tex.surface);
            tex.image = nullptr;
            tex.surface = nullptr;
        }

        void use_ogl_texture(const OGLTexture& tex, GLint tex_loc) {
            glActiveTexture(priv::get_texture_unit(tex.index));
            glBindTexture(tex.target, tex.buffer);
            glUniform1i(tex_loc, tex.index);
        }

    }
}
