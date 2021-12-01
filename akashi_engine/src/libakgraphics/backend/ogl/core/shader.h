#pragma once

#include "./glc.h"

namespace akashi {
    namespace graphics {

        bool compile_attach_shader(const GLuint prog, const GLenum type, const char* source);

        bool link_shader(const GLuint prog);

    }
}
