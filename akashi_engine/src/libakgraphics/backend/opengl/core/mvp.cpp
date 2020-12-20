#include "./mvp.h"

#include "../gl.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        void update_translate(const GLRenderContext& ctx, const core::LayerContext& layer_ctx,
                              glm::mat4& new_mvp) {
            GLint viewport[4];
            GET_GLFUNC(ctx, glGetIntegerv)(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];

            auto c_x = Rational(screen_width, 1) / 2;
            auto c_y = Rational(screen_height, 1) / 2;

            auto a_x = Rational(layer_ctx.x); // mouse coord
            auto a_y = Rational(layer_ctx.y); // mouse coord

            new_mvp = glm::translate(
                new_mvp,
                glm::vec3((Rational(2l) * (a_x - c_x) / screen_width).to_decimal(),
                          (Rational(-2l) * (a_y - c_y) / screen_height).to_decimal(), 0.0));
        }

        void update_scale(const GLRenderContext& ctx, const GLTextureData& tex,
                          glm::mat4& new_mvp) {
            // adjustment of aspect ratio
            GLint viewport[4];
            GET_GLFUNC(ctx, glGetIntegerv)(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];
            Rational aspect = Rational(tex.effective_width, 1) / Rational(tex.effective_height, 1);
            Rational scale_w = Rational(1l);
            Rational scale_h = Rational(1l);

            if (tex.effective_width < screen_width && tex.height < screen_height) {
                scale_w = Rational(tex.effective_width, 1) / Rational(screen_width, 1);
                scale_h = Rational(tex.effective_height, 1) / Rational(screen_height, 1);
            } else if (screen_width > screen_height) {
                // fixed height
                scale_w = (Rational(screen_height, 1) * aspect) / Rational(screen_width, 1);
                scale_h = Rational(1l);
                if (scale_w > Rational(1l)) {
                    scale_h /= scale_w;
                    scale_w = Rational(1l);
                }
            } else {
                // fixed width
                scale_w = Rational(1l);
                scale_h = (Rational(screen_width, 1) / aspect) / Rational(screen_height, 1);
                if (scale_h > Rational(1l)) {
                    scale_w /= scale_h;
                    scale_h = Rational(1l);
                }
            }

            new_mvp =
                glm::scale(new_mvp, glm::vec3(scale_w.to_decimal(), scale_h.to_decimal(), 1.0));
        }

    }
}
