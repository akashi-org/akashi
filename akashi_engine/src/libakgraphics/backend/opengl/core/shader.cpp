#include "./shader.h"

#include <libakcore/logger.h>

#include <string>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        bool compile_attach_shader(const GLuint prog, const GLenum type, const char* source) {
            bool res = true;
            GLuint shader = 0;
            GLint compile_status = 0;
            GLint log_length = 0;
            GLchar* log_str = nullptr;

            shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

            if (log_length > 1) {
                log_str = static_cast<GLchar*>(calloc(log_length + 1, sizeof(GLchar)));
                glGetShaderInfoLog(shader, log_length, nullptr, log_str);
                AKLOG_ERROR("Shader compile log: {}", log_str);
            }

            if (!compile_status) {
                AKLOG_ERROR("compile_attach_shader()<type={}> failed: Failed to compile shader: {}",
                            type, compile_status);
                res = false;
                goto exit;
            }

            glAttachShader(prog, shader);

        exit:
            if (log_str != nullptr) {
                free(log_str);
                log_str = nullptr;
            }
            glDeleteShader(shader);
            return res;
        }

        bool link_shader(const GLuint prog) {
            GLint link_status = 0;
            GLint log_length = 0;

            glLinkProgram(prog);

            glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);

            if (!link_status) {
                GLchar* log_str = static_cast<GLchar*>(calloc(log_length + 1, sizeof(GLchar)));
                glGetProgramInfoLog(prog, log_length, nullptr, log_str);
                AKLOG_ERROR("link_program() failed: Failed to link shader: {}, {}", link_status,
                            log_str);
                free(log_str);
                return false;
            } else {
                return true;
            }
        }

    }
}
