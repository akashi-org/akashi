#include "./mvp.h"

#include "../gl.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>

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

            int c_x = screen_width / 2;
            int c_y = screen_height / 2;

            double a_x = layer_ctx.x; // mouse coord
            double a_y = layer_ctx.y; // mouse coord

            new_mvp = glm::translate(new_mvp, glm::vec3(2 * (a_x - c_x) / screen_width,
                                                        -2 * (a_y - c_y) / screen_height, 0.0));
        }

        void update_scale(const GLRenderContext& ctx, const GLTextureData& tex,
                          glm::mat4& new_mvp) {
            // [TODO] it looks like there might be problems with numerical errors. we should use
            // rational here.

            // adjustment of aspect ratio
            GLint viewport[4];
            GET_GLFUNC(ctx, glGetIntegerv)(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];
            double aspect = (double)tex.width / tex.height;
            double scale_w = 1.0;
            double scale_h = 1.0;

            if (tex.width < screen_width && tex.height < screen_height) {
                scale_w = (double)tex.width / screen_width;
                scale_h = (double)tex.height / screen_height;
            } else if (screen_width > screen_height) {
                // fixed height
                scale_w = (screen_height * aspect) / screen_width;
                scale_h = 1.0;
                if (scale_w > 1) {
                    scale_h /= scale_w;
                    scale_w = 1.0;
                }
            } else {
                // fixed width
                scale_w = 1.0;
                scale_h = (screen_width / aspect) / screen_height;
                if (scale_h > 1) {
                    scale_w /= scale_h;
                    scale_h = 1.0;
                }
            }

            new_mvp = glm::scale(new_mvp, glm::vec3(scale_w, scale_h, 1.0));
        }

    }
}
