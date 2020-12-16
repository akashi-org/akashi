#pragma once

#include <GL/gl.h>

namespace akashi {
    namespace graphics {

        struct GLRenderContext;

        bool compile_attach_shader(const GLRenderContext& ctx, const GLuint prog, const GLenum type,
                                   const char* source);

        bool link_shader(const GLRenderContext& ctx, const GLuint prog);

    }
}
