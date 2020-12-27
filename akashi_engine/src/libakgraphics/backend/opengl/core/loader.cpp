#include "./loader.h"
#include "../../../item.h"

#include "../gl.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <string>

using namespace akashi::core;

#define DEF_FN(ctx, proc_addr, func_name)                                                          \
    set_gl_function(proc_addr.func(nullptr, #func_name), GET_GLFUNC(ctx, func_name), #func_name)

#define DEF_FN_CHECK(ctx, proc_addr, func_name) CHECK_AK_ERROR(DEF_FN(ctx, proc_addr, func_name))

namespace akashi {
    namespace graphics {

        template <class T>
        static bool set_gl_function(void* func, T& func_slot, const char* func_name) {
            if (func == nullptr) {
                AKLOG_ERROR("set_gl_function() failed: Could not load OpenGL functions: {}",
                            func_name);
                return false;
            }
            func_slot = reinterpret_cast<T>(func);
            return true;
        }

        bool parse_gl_version(GLRenderContext& ctx) {
            const char* version_string = nullptr;
            int major = 0;
            int minor = 0;
            auto GetString = GET_GLFUNC(ctx, glGetString);

            version_string = (char*)(GetString(GL_VERSION));
            if (!version_string) {
                AKLOG_ERRORN("parse_gl_version() failed: glGetString(GL_VERSION) returned NULL");
                return false;
            }
            AKLOG_INFO("GL_VERSION_STRING: {}", version_string);

            // [TODO] is it security-safe?
            if (sscanf(version_string, "%d.%d", &major, &minor) < 2) {
                AKLOG_ERRORN("parse_gl_version() failed: Failed to parse GL_VERSION_STRING");
                return false;
            }

            ctx.version = (major * 100) + (minor * 10);
            AKLOG_INFO("GL_VERSION: {}", ctx.version);

            AKLOG_DEBUG("GL_VENDOR: {}", GetString(GL_VENDOR));
            AKLOG_DEBUG("GL_RENDERER: {}", GetString(GL_RENDERER));

            return true;
        }

        bool parse_shader(GLRenderContext& ctx) {
            const char* shader = nullptr;
            ctx.glsl_version = 120;
            int glsl_major = 0;
            int glsl_minor = 0;
            auto GetString = GET_GLFUNC(ctx, glGetString);

            shader = (char*)(GetString(GL_SHADING_LANGUAGE_VERSION));
            if (shader) {
                AKLOG_INFO("GL_SHADING_LANGUAGE_VERSION: {}", shader);
            }

            // [TODO] is it security-safe?
            if (shader && sscanf(shader, "%d.%d", &glsl_major, &glsl_minor) == 2) {
                ctx.glsl_version = glsl_major * 100 + glsl_minor;
            }
            ctx.glsl_version = ctx.glsl_version > 440 ? 440 : ctx.glsl_version;

            AKLOG_INFO("GLSL_VERSION: {}", ctx.glsl_version);

            return true;
        }

        bool parse_gl_extensions(const GetProcAddress& get_proc_address, GLRenderContext& ctx) {
            auto GetString = GET_GLFUNC(ctx, glGetString);

            if (ctx.version >= 300) {
                if (!DEF_FN(ctx, get_proc_address, glGetStringi)) {
                    return false;
                }
                if (!DEF_FN(ctx, get_proc_address, glGetIntegerv)) {
                    return false;
                }

                GLint exts;
                GET_GLFUNC(ctx, glGetIntegerv)(GL_NUM_EXTENSIONS, &exts);
                for (int n = 0; n < exts; n++) {
                    const char* ext = (char*)(GET_GLFUNC(ctx, glGetStringi(GL_EXTENSIONS, n)));
                    ctx.extensions.append(" " + std::string(ext));
                }

            } else {
                const char* ext = (char*)(GetString(GL_EXTENSIONS));
                ctx.extensions.append(" " + std::string(ext));
            }

            AKLOG_INFO("GL_EXTENSION_STRING:{}", ctx.extensions);

            return true;
        }

        ak_error_t load_gl_functions(const GetProcAddress& get_proc_address, GLRenderContext& ctx) {
            DEF_FN_CHECK(ctx, get_proc_address, glViewport);
            DEF_FN_CHECK(ctx, get_proc_address, glClear);
            DEF_FN_CHECK(ctx, get_proc_address, glClearColor);
            DEF_FN_CHECK(ctx, get_proc_address, glEnable);
            DEF_FN_CHECK(ctx, get_proc_address, glDisable);
            DEF_FN_CHECK(ctx, get_proc_address, glGetString);
            DEF_FN_CHECK(ctx, get_proc_address, glUseProgram);
            DEF_FN_CHECK(ctx, get_proc_address, glGetUniformLocation);

            DEF_FN_CHECK(ctx, get_proc_address, glUniform1f);
            DEF_FN_CHECK(ctx, get_proc_address, glUniform2f);
            DEF_FN_CHECK(ctx, get_proc_address, glUniform3f);
            DEF_FN_CHECK(ctx, get_proc_address, glUniform4f);
            DEF_FN_CHECK(ctx, get_proc_address, glUniform1i);
            DEF_FN_CHECK(ctx, get_proc_address, glUniformMatrix2fv);
            DEF_FN_CHECK(ctx, get_proc_address, glUniformMatrix3fv);
            DEF_FN_CHECK(ctx, get_proc_address, glUniformMatrix4fv);

            DEF_FN_CHECK(ctx, get_proc_address, glGenTextures);
            DEF_FN_CHECK(ctx, get_proc_address, glDeleteTextures);
            DEF_FN_CHECK(ctx, get_proc_address, glTexImage2D);
            DEF_FN_CHECK(ctx, get_proc_address, glTexSubImage2D);
            DEF_FN_CHECK(ctx, get_proc_address, glTexParameteri);

            DEF_FN_CHECK(ctx, get_proc_address, glCompileShader);
            DEF_FN_CHECK(ctx, get_proc_address, glCreateProgram);
            DEF_FN_CHECK(ctx, get_proc_address, glCreateShader);
            DEF_FN_CHECK(ctx, get_proc_address, glShaderSource);
            DEF_FN_CHECK(ctx, get_proc_address, glLinkProgram);
            DEF_FN_CHECK(ctx, get_proc_address, glAttachShader);
            DEF_FN_CHECK(ctx, get_proc_address, glDeleteShader);
            DEF_FN_CHECK(ctx, get_proc_address, glDeleteProgram);
            DEF_FN_CHECK(ctx, get_proc_address, glGetShaderInfoLog);
            DEF_FN_CHECK(ctx, get_proc_address, glGetShaderiv);
            DEF_FN_CHECK(ctx, get_proc_address, glGetProgramInfoLog);
            DEF_FN_CHECK(ctx, get_proc_address, glGetProgramiv);
            DEF_FN_CHECK(ctx, get_proc_address, glGetStringi);
            DEF_FN_CHECK(ctx, get_proc_address, glBindAttribLocation);
            DEF_FN_CHECK(ctx, get_proc_address, glGetTranslatedShaderSourceANGLE);

            DEF_FN_CHECK(ctx, get_proc_address, glFlush);
            DEF_FN_CHECK(ctx, get_proc_address, glFinish);
            DEF_FN_CHECK(ctx, get_proc_address, glDrawArrays);
            DEF_FN_CHECK(ctx, get_proc_address, glDrawElements);
            DEF_FN_CHECK(ctx, get_proc_address, glGenBuffers);
            DEF_FN_CHECK(ctx, get_proc_address, glDeleteBuffers);
            DEF_FN_CHECK(ctx, get_proc_address, glBindBuffer);
            DEF_FN_CHECK(ctx, get_proc_address, glBindBufferBase);
            DEF_FN_CHECK(ctx, get_proc_address, glMapBufferRange);
            DEF_FN_CHECK(ctx, get_proc_address, glUnmapBuffer);
            DEF_FN_CHECK(ctx, get_proc_address, glBufferData);
            DEF_FN_CHECK(ctx, get_proc_address, glBufferSubData);
            DEF_FN_CHECK(ctx, get_proc_address, glActiveTexture);
            DEF_FN_CHECK(ctx, get_proc_address, glBindTexture);
            DEF_FN_CHECK(ctx, get_proc_address, glSwapInterval);
            DEF_FN_CHECK(ctx, get_proc_address, glTexImage3D);
            DEF_FN_CHECK(ctx, get_proc_address, glGenVertexArrays);
            DEF_FN_CHECK(ctx, get_proc_address, glBindVertexArray);
            DEF_FN_CHECK(ctx, get_proc_address, glGetAttribLocation);
            DEF_FN_CHECK(ctx, get_proc_address, glEnableVertexAttribArray);
            DEF_FN_CHECK(ctx, get_proc_address, glDisableVertexAttribArray);
            DEF_FN_CHECK(ctx, get_proc_address, glVertexAttribPointer);
            DEF_FN_CHECK(ctx, get_proc_address, glDeleteVertexArrays);
            DEF_FN_CHECK(ctx, get_proc_address, glGenerateMipmap);
            DEF_FN_CHECK(ctx, get_proc_address, glBlendFunc);

            DEF_FN_CHECK(ctx, get_proc_address, glBindFramebuffer);
            DEF_FN_CHECK(ctx, get_proc_address, glGenFramebuffers);
            DEF_FN_CHECK(ctx, get_proc_address, glDeleteFramebuffers);
            DEF_FN_CHECK(ctx, get_proc_address, glCheckFramebufferStatus);
            DEF_FN_CHECK(ctx, get_proc_address, glFramebufferTexture2D);
            DEF_FN_CHECK(ctx, get_proc_address, glBlitFramebuffer);
            DEF_FN_CHECK(ctx, get_proc_address, glGetFramebufferAttachmentParameteriv);

            DEF_FN_CHECK(ctx, get_proc_address, glDrawBuffers);

            DEF_FN_CHECK(ctx, get_proc_address, glGenRenderbuffers);
            DEF_FN_CHECK(ctx, get_proc_address, glBindRenderbuffer);
            DEF_FN_CHECK(ctx, get_proc_address, glRenderbufferStorage);
            DEF_FN_CHECK(ctx, get_proc_address, glFramebufferRenderbuffer);

            return ErrorType::OK;
        }

        ak_error_t load_gl_getString(const GetProcAddress& get_proc_address, GLRenderContext& ctx) {
            DEF_FN_CHECK(ctx, get_proc_address, glGetString);
            return ErrorType::OK;
        }

    }
}
