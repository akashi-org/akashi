#include "./glc.h"

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

        static std::string get_mangled_func(const std::string& trace_str) {
            auto begin_tk = trace_str.find("(_Z");
            auto end_tk = trace_str.find("+0x", begin_tk + 1);
            return (begin_tk == trace_str.npos || end_tk == trace_str.npos)
                       ? ""
                       : trace_str.substr(begin_tk + 1, end_tk - begin_tk - 1);
        }

        static std::string collect_stacktrace(const int max_stack_size) {
            std::string stacktrace_str;
            stacktrace_str += "\nStackTrace: \n";
            void* array[max_stack_size];
            size_t size = 0;
            char** trace_strs;
            size = backtrace(array, max_stack_size);
            trace_strs = backtrace_symbols(array, size);
            if (trace_strs) {
                for (size_t i = 0; i < size; i++) {
                    stacktrace_str += "  #" + std::to_string(i) + " ";
                    int status = -1;
                    auto demangled_trace = abi::__cxa_demangle(
                        get_mangled_func(trace_strs[i]).c_str(), nullptr, 0, &status);
                    if (status == 0 && demangled_trace) {
                        stacktrace_str += demangled_trace;
                        free(demangled_trace);
                    } else {
                        stacktrace_str += trace_strs[i];
                    }
                    if (i != size - 1) {
                        stacktrace_str += "\n";
                    }
                }
            }
            free(trace_strs);

            return stacktrace_str;
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
            if (type_str == "Error" || type_str == "Undefined Behavior") {
                stacktrace_str = collect_stacktrace(7);
            }

            AKLOG_RLOG(log_level, "OGL_DEBUG({},{},{}): {}{}", id, type_str, severity_str, message,
                       stacktrace_str);

            // if (std::string(message).find("debug marker") != std::string::npos) {
            //     AKLOG_ERRORN("!!!");
            // }
        }

    }
}
