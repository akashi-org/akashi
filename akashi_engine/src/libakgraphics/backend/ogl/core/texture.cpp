#include "./texture.h"

#include <libakcore/rational.h>
#include <libakcore/logger.h>

#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

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

        glm::vec3 get_sar_scale_vec(const OGLTexture& tex, const double scale_ratio) {
            // adjustment of aspect ratio
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];
            core::Rational aspect =
                core::Rational(tex.effective_width, 1) / core::Rational(tex.effective_height, 1);
            core::Rational scale_w = core::Rational(1l);
            core::Rational scale_h = core::Rational(1l);

            if (tex.effective_width < screen_width && tex.height < screen_height) {
                scale_w = core::Rational(tex.effective_width, 1) / core::Rational(screen_width, 1);
                scale_h =
                    core::Rational(tex.effective_height, 1) / core::Rational(screen_height, 1);

            } else if (screen_width > screen_height) {
                // fixed height
                scale_w =
                    (core::Rational(screen_height, 1) * aspect) / core::Rational(screen_width, 1);
                scale_h = core::Rational(1l);
                if (scale_w > core::Rational(1l)) {
                    scale_h /= scale_w;
                    scale_w = core::Rational(1l);
                }
            } else {
                // fixed width
                scale_w = core::Rational(1l);
                scale_h =
                    (core::Rational(screen_width, 1) / aspect) / core::Rational(screen_height, 1);
                if (scale_h > core::Rational(1l)) {
                    scale_w /= scale_h;
                    scale_h = core::Rational(1l);
                }
            }

            return glm::vec3(scale_w.to_decimal(), scale_h.to_decimal(), 1.0) * (float)scale_ratio;
        }

    }
}
