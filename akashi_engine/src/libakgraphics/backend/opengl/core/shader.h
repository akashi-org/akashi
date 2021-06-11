#pragma once

#include <GL/gl.h>
#include <string>

namespace akashi {
    namespace core {
        struct LayerContext;
        enum class LayerType;
    }
    namespace graphics {

        struct GLRenderContext;

        bool compile_attach_shader(const GLRenderContext& ctx, const GLuint prog, const GLenum type,
                                   const char* source);

        bool link_shader(const GLRenderContext& ctx, const GLuint prog);

    }
}
