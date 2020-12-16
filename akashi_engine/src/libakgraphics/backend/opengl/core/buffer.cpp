#include "./buffer.h"

#include "../gl.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        void create_buffer(const GLRenderContext& ctx, GLuint& buffer, GLenum target, void* data,
                           size_t data_size) {
            // Generally, error check is not necessary in this case
            // ref: https://stackoverflow.com/questions/20897179/behavior-of-glgenbuffers
            GET_GLFUNC(ctx, glGenBuffers)(1, &buffer);

            GET_GLFUNC(ctx, glBindBuffer)(target, buffer);
            GET_GLFUNC(ctx, glBufferData)(target, data_size, data, GL_STATIC_DRAW);

            GET_GLFUNC(ctx, glBindBuffer)(target, 0);
        }

    }
}
