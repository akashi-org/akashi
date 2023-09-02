#include "./glc.h"

#include <libakcore/utils.h>
#include <libakcore/logger.h>

#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        const char* gl_err_to_str(GLenum err) {
            switch (err) {
                case GL_NO_ERROR:
                    return "GL_NO_ERROR";
                case GL_INVALID_ENUM:
                    return "GL_INVALID_ENUM";
                case GL_INVALID_VALUE:
                    return "GL_INVALID_VALUE";
                case GL_INVALID_OPERATION:
                    return "GL_INVALID_OPERATION";
                case GL_STACK_OVERFLOW:
                    return "GL_STACK_OVERFLOW";
                case GL_STACK_UNDERFLOW:
                    return "GL_STACK_UNDERFLOW";
                case GL_OUT_OF_MEMORY:
                    return "GL_OUT_OF_MEMORY";
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    return "GL_INVALID_FRAMEBUFFER_OPERATION";

                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    return "GL_FRAMEBUFFER_UNSUPPORTED";
                default: {
                    return "unknown error";
                    break;
                }
            }
        }

        void gl_debug_message_callback(GLenum /*source*/, GLenum type, unsigned int id,
                                       GLenum severity, GLsizei /*length*/, const char* message,
                                       const void* /*userParam*/) {
            core::LogLevel log_level = LogLevel::DEBUG;
            std::string type_str;
            switch (type) {
                case GL_DEBUG_TYPE_ERROR_ARB:
                    type_str = "Error";
                    log_level = LogLevel::ERROR;
                    break;
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
                    type_str = "Deprecated Behavior";
                    log_level = LogLevel::ERROR;
                    break;
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
                    type_str = "Undefined Behavior";
                    log_level = LogLevel::ERROR;
                    break;
                case GL_DEBUG_TYPE_PORTABILITY_ARB:
                    type_str = "Portability";
                    log_level = LogLevel::WARN;
                    break;
                case GL_DEBUG_TYPE_PERFORMANCE_ARB:
                    type_str = "Performance";
                    log_level = LogLevel::INFO;
                    break;
            }

            std::string severity_str;
            switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH_ARB:
                    severity_str = "High";
                    break;
                case GL_DEBUG_SEVERITY_MEDIUM_ARB:
                    severity_str = "Medium";
                    break;
                case GL_DEBUG_SEVERITY_LOW_ARB:
                    severity_str = "Low";
                    break;
                default:
                    severity_str = "Unknown";
                    break;
            }

            if (type_str.empty()) {
                return;
            }

            std::string stacktrace_str;
            stacktrace_str = core::collect_stacktrace(12);

            AKLOG_RLOG(log_level, "OGL_DEBUG({},{},{}): {}{}", id, type_str, severity_str, message,
                       stacktrace_str);

            // if (std::string(message).find("debug marker") != std::string::npos) {
            //     AKLOG_ERRORN("!!!");
            // }
        }

    }
}
