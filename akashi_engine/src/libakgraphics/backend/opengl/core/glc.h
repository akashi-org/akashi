#pragma once

#include "./glc_glad.h"
// #include "./glc_custom.h"

#define CHECK_GL_ERRORS()                                                                          \
    do {                                                                                           \
        GLenum err;                                                                                \
        bool no_error = true;                                                                      \
        while ((err = glGetError()) != GL_NO_ERROR) {                                              \
            no_error = false;                                                                      \
            AKLOG_ERROR("OpenGL Error: 0x{:x}, {}", err, akashi::graphics::gl_err_to_str(err));    \
        }                                                                                          \
        if (!no_error) {                                                                           \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

namespace akashi {
    namespace graphics {
        const char* gl_err_to_str(GLenum gl_err);

        void gl_debug_message_callback(GLenum source, GLenum type, unsigned int id, GLenum severity,
                                       GLsizei length, const char* message, const void* userParam);

    }
}
