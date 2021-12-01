#include "./loader.h"
#include "../../../item.h"

#include "./glc.h"

#include <libakcore/logger.h>

#include <string>

using namespace akashi::core;

#ifdef AK_GL_CUSTOM_LOADER

#define DEF_FN_CHECK(proc_addr, func_name)                                                         \
    do {                                                                                           \
        auto func = proc_addr.func(#func_name);                                                    \
        if (!func) {                                                                               \
            AKLOG_ERROR("Could not load OpenGL function: {}", func_name);                          \
            return false;                                                                          \
        }                                                                                          \
        akashi::graphics::func_name =                                                              \
            reinterpret_cast<decltype(akashi::graphics::func_name)>(func);                         \
    } while (0)

AK_GL_DECL_FUNCS(AK_VOID)

namespace akashi {
    namespace graphics {

        bool load_gl_functions(const GetProcAddress& get_proc_address) {
            DEF_FN_CHECK(get_proc_address, glGetString);
            DEF_FN_CHECK(get_proc_address, glGetStringi);
            DEF_FN_CHECK(get_proc_address, glGetIntegerv);
            DEF_FN_CHECK(get_proc_address, glViewport);
            DEF_FN_CHECK(get_proc_address, glClear);
            DEF_FN_CHECK(get_proc_address, glClearColor);
            DEF_FN_CHECK(get_proc_address, glEnable);
            DEF_FN_CHECK(get_proc_address, glDisable);
            DEF_FN_CHECK(get_proc_address, glGetString);
            DEF_FN_CHECK(get_proc_address, glUseProgram);
            DEF_FN_CHECK(get_proc_address, glGetUniformLocation);
            DEF_FN_CHECK(get_proc_address, glGetError);

            DEF_FN_CHECK(get_proc_address, glDepthFunc);
            DEF_FN_CHECK(get_proc_address, glCullFace);

            DEF_FN_CHECK(get_proc_address, glUniform1f);
            DEF_FN_CHECK(get_proc_address, glUniform2f);
            DEF_FN_CHECK(get_proc_address, glUniform3f);
            DEF_FN_CHECK(get_proc_address, glUniform4f);
            DEF_FN_CHECK(get_proc_address, glUniform1i);
            DEF_FN_CHECK(get_proc_address, glUniformMatrix2fv);
            DEF_FN_CHECK(get_proc_address, glUniformMatrix3fv);
            DEF_FN_CHECK(get_proc_address, glUniformMatrix4fv);

            DEF_FN_CHECK(get_proc_address, glGenTextures);
            DEF_FN_CHECK(get_proc_address, glDeleteTextures);
            DEF_FN_CHECK(get_proc_address, glTexImage2D);
            DEF_FN_CHECK(get_proc_address, glTexSubImage2D);
            DEF_FN_CHECK(get_proc_address, glTexParameteri);

            DEF_FN_CHECK(get_proc_address, glCompileShader);
            DEF_FN_CHECK(get_proc_address, glCreateProgram);
            DEF_FN_CHECK(get_proc_address, glCreateShader);
            DEF_FN_CHECK(get_proc_address, glShaderSource);
            DEF_FN_CHECK(get_proc_address, glLinkProgram);
            DEF_FN_CHECK(get_proc_address, glAttachShader);
            DEF_FN_CHECK(get_proc_address, glDeleteShader);
            DEF_FN_CHECK(get_proc_address, glDeleteProgram);
            DEF_FN_CHECK(get_proc_address, glGetShaderInfoLog);
            DEF_FN_CHECK(get_proc_address, glGetShaderiv);
            DEF_FN_CHECK(get_proc_address, glGetProgramInfoLog);
            DEF_FN_CHECK(get_proc_address, glGetProgramiv);
            DEF_FN_CHECK(get_proc_address, glGetStringi);
            DEF_FN_CHECK(get_proc_address, glBindAttribLocation);
            DEF_FN_CHECK(get_proc_address, glGetTranslatedShaderSourceANGLE);

            DEF_FN_CHECK(get_proc_address, glFlush);
            DEF_FN_CHECK(get_proc_address, glFinish);
            DEF_FN_CHECK(get_proc_address, glDrawArrays);
            DEF_FN_CHECK(get_proc_address, glDrawElements);
            DEF_FN_CHECK(get_proc_address, glGenBuffers);
            DEF_FN_CHECK(get_proc_address, glDeleteBuffers);
            DEF_FN_CHECK(get_proc_address, glBindBuffer);
            DEF_FN_CHECK(get_proc_address, glBindBufferBase);
            DEF_FN_CHECK(get_proc_address, glMapBufferRange);
            DEF_FN_CHECK(get_proc_address, glUnmapBuffer);
            DEF_FN_CHECK(get_proc_address, glBufferData);
            DEF_FN_CHECK(get_proc_address, glBufferSubData);
            DEF_FN_CHECK(get_proc_address, glActiveTexture);
            DEF_FN_CHECK(get_proc_address, glBindTexture);
            DEF_FN_CHECK(get_proc_address, glSwapInterval);
            DEF_FN_CHECK(get_proc_address, glTexImage3D);
            DEF_FN_CHECK(get_proc_address, glTexSubImage3D);
            DEF_FN_CHECK(get_proc_address, glTexStorage3D);
            DEF_FN_CHECK(get_proc_address, glGenVertexArrays);
            DEF_FN_CHECK(get_proc_address, glBindVertexArray);
            DEF_FN_CHECK(get_proc_address, glGetAttribLocation);
            DEF_FN_CHECK(get_proc_address, glEnableVertexAttribArray);
            DEF_FN_CHECK(get_proc_address, glDisableVertexAttribArray);
            DEF_FN_CHECK(get_proc_address, glVertexAttribPointer);
            DEF_FN_CHECK(get_proc_address, glDeleteVertexArrays);
            DEF_FN_CHECK(get_proc_address, glGenerateMipmap);
            DEF_FN_CHECK(get_proc_address, glBlendFunc);

            DEF_FN_CHECK(get_proc_address, glBindFramebuffer);
            DEF_FN_CHECK(get_proc_address, glGenFramebuffers);
            DEF_FN_CHECK(get_proc_address, glDeleteFramebuffers);
            DEF_FN_CHECK(get_proc_address, glCheckFramebufferStatus);
            DEF_FN_CHECK(get_proc_address, glFramebufferTexture2D);
            DEF_FN_CHECK(get_proc_address, glBlitFramebuffer);
            DEF_FN_CHECK(get_proc_address, glGetFramebufferAttachmentParameteriv);

            DEF_FN_CHECK(get_proc_address, glDrawBuffers);

            DEF_FN_CHECK(get_proc_address, glGenRenderbuffers);
            DEF_FN_CHECK(get_proc_address, glBindRenderbuffer);
            DEF_FN_CHECK(get_proc_address, glRenderbufferStorage);
            DEF_FN_CHECK(get_proc_address, glFramebufferRenderbuffer);

            DEF_FN_CHECK(get_proc_address, glPixelStorei);
            DEF_FN_CHECK(get_proc_address, glReadPixels);
            DEF_FN_CHECK(get_proc_address, glReadBuffer);

            return true;
        }

    }
}

#endif
