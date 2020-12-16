#pragma once

#include <GL/gl.h>
#include <cstdlib>

namespace akashi {
    namespace graphics {

        struct GLRenderContext;
        void create_buffer(const GLRenderContext& ctx, GLuint& buffer, GLenum target, void* data,
                           size_t data_size);

    }
}
