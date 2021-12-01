#pragma once
#include <glad/glad.h>

// https://github.com/Dav1dde/glad/issues/58
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504

namespace akashi {
    namespace graphics {
        const char* gl_err_to_str(GLenum gl_err);
        bool check_gl_errors();
    }
}
