#pragma once

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>

#define AK_GL_CUSTOM_LOADER 1;

#define AK_VOID ;

#define AK_GL_DECL_FUNCS(decl_kind)                                                                \
    namespace akashi {                                                                             \
        namespace graphics {                                                                       \
                                                                                                   \
            decl_kind void(GLAPIENTRY * glCullFace)(GLenum);                                       \
            decl_kind void(GLAPIENTRY * glDepthFunc)(GLenum);                                      \
            decl_kind void(GLAPIENTRY * glGenerateMipmap)(GLenum);                                 \
            decl_kind void(GLAPIENTRY * glBlendFunc)(GLenum, GLenum);                              \
            decl_kind void(GLAPIENTRY * glViewport)(GLint, GLint, GLsizei, GLsizei);               \
            decl_kind void(GLAPIENTRY * glClear)(GLbitfield);                                      \
            decl_kind void(GLAPIENTRY * glGenTextures)(GLsizei, GLuint*);                          \
            decl_kind void(GLAPIENTRY * glDeleteTextures)(GLsizei, const GLuint*);                 \
            decl_kind void(GLAPIENTRY * glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);     \
            decl_kind void(GLAPIENTRY * glEnable)(GLenum);                                         \
            decl_kind void(GLAPIENTRY * glDisable)(GLenum);                                        \
            decl_kind const GLubyte*(GLAPIENTRY * glGetString)(GLenum);                            \
            decl_kind void(GLAPIENTRY * glFlush)(void);                                            \
            decl_kind void(GLAPIENTRY * glFinish)(void);                                           \
            decl_kind void(GLAPIENTRY * glPixelStorei)(GLenum, GLint);                             \
            decl_kind void(GLAPIENTRY * glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei,      \
                                                      GLint, GLenum, GLenum, const GLvoid*);       \
            decl_kind void(GLAPIENTRY * glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei,     \
                                                         GLsizei, GLenum, GLenum, const GLvoid*);  \
            decl_kind void(GLAPIENTRY * glTexParameteri)(GLenum, GLenum, GLint);                   \
            decl_kind void(GLAPIENTRY * glGetIntegerv)(GLenum, GLint*);                            \
            decl_kind void(GLAPIENTRY * glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum,      \
                                                      GLenum, GLvoid*);                            \
            decl_kind void(GLAPIENTRY * glReadBuffer)(GLenum);                                     \
            decl_kind void(GLAPIENTRY * glDrawArrays)(GLenum, GLint, GLsizei);                     \
            decl_kind void(GLAPIENTRY * glDrawElements)(GLenum mode, GLsizei count, GLenum type,   \
                                                        const void* indices);                      \
            decl_kind GLenum(GLAPIENTRY* glGetError)(void);                                        \
            decl_kind void(GLAPIENTRY * glGenBuffers)(GLsizei, GLuint*);                           \
            decl_kind void(GLAPIENTRY * glDeleteBuffers)(GLsizei, const GLuint*);                  \
            decl_kind void(GLAPIENTRY * glBindBuffer)(GLenum, GLuint);                             \
            decl_kind void(GLAPIENTRY * glBindBufferBase)(GLenum, GLuint, GLuint);                 \
            decl_kind GLvoid*(GLAPIENTRY * glMapBufferRange)(GLenum, GLintptr, GLsizeiptr,         \
                                                             GLbitfield);                          \
            decl_kind GLboolean(GLAPIENTRY* glUnmapBuffer)(GLenum);                                \
            decl_kind void(GLAPIENTRY * glBufferData)(GLenum, intptr_t, const GLvoid*, GLenum);    \
                                                                                                   \
            decl_kind void(GLAPIENTRY * glBufferSubData)(GLenum, GLintptr, GLsizeiptr,             \
                                                         const GLvoid*);                           \
            decl_kind void(GLAPIENTRY * glActiveTexture)(GLenum);                                  \
            decl_kind void(GLAPIENTRY * glBindTexture)(GLenum, GLuint);                            \
            decl_kind int(GLAPIENTRY * glSwapInterval)(int);                                       \
            decl_kind void(GLAPIENTRY * glTexImage3D)(GLenum, GLint, GLenum, GLsizei, GLsizei,     \
                                                      GLsizei, GLint, GLenum, GLenum,              \
                                                      const GLvoid*);                              \
                                                                                                   \
            decl_kind void(GLAPIENTRY * glTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint,       \
                                                         GLsizei, GLsizei, GLsizei, GLenum,        \
                                                         GLenum, const void*);                     \
                                                                                                   \
            decl_kind void(GLAPIENTRY * glTexStorage3D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, \
                                                        GLsizei);                                  \
                                                                                                   \
            decl_kind void(GLAPIENTRY * glGenVertexArrays)(GLsizei, GLuint*);                      \
            decl_kind void(GLAPIENTRY * glBindVertexArray)(GLuint);                                \
            decl_kind GLint(GLAPIENTRY* glGetAttribLocation)(GLuint, const GLchar*);               \
            decl_kind void(GLAPIENTRY * glEnableVertexAttribArray)(GLuint);                        \
            decl_kind void(GLAPIENTRY * glDisableVertexAttribArray)(GLuint);                       \
            decl_kind void(GLAPIENTRY * glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean,   \
                                                               GLsizei, const GLvoid*);            \
            decl_kind void(GLAPIENTRY * glDeleteVertexArrays)(GLsizei, const GLuint*);             \
            decl_kind void(GLAPIENTRY * glUseProgram)(GLuint);                                     \
            decl_kind GLint(GLAPIENTRY* glGetUniformLocation)(GLuint, const GLchar*);              \
            decl_kind void(GLAPIENTRY * glCompileShader)(GLuint);                                  \
            decl_kind GLuint(GLAPIENTRY* glCreateProgram)(void);                                   \
            decl_kind GLuint(GLAPIENTRY* glCreateShader)(GLenum);                                  \
            decl_kind void(GLAPIENTRY * glShaderSource)(GLuint, GLsizei, const GLchar**,           \
                                                        const GLint*);                             \
            decl_kind void(GLAPIENTRY * glLinkProgram)(GLuint);                                    \
            decl_kind void(GLAPIENTRY * glAttachShader)(GLuint, GLuint);                           \
            decl_kind void(GLAPIENTRY * glDeleteShader)(GLuint);                                   \
            decl_kind void(GLAPIENTRY * glDeleteProgram)(GLuint);                                  \
            decl_kind void(GLAPIENTRY * glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);   \
            decl_kind void(GLAPIENTRY * glGetShaderiv)(GLuint, GLenum, GLint*);                    \
            decl_kind void(GLAPIENTRY * glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);  \
            decl_kind void(GLAPIENTRY * glGetProgramiv)(GLenum, GLenum, GLint*);                   \
            decl_kind const GLubyte*(GLAPIENTRY * glGetStringi)(GLenum, GLuint);                   \
            decl_kind void(GLAPIENTRY * glBindAttribLocation)(GLuint, GLuint, const GLchar*);      \
            decl_kind void(GLAPIENTRY * glBindFramebuffer)(GLenum, GLuint);                        \
            decl_kind void(GLAPIENTRY * glGenFramebuffers)(GLsizei, GLuint*);                      \
            decl_kind void(GLAPIENTRY * glDeleteFramebuffers)(GLsizei, const GLuint*);             \
            decl_kind GLenum(GLAPIENTRY* glCheckFramebufferStatus)(GLenum);                        \
            decl_kind void(GLAPIENTRY * glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint,    \
                                                                GLint);                            \
            decl_kind void(GLAPIENTRY * glBlitFramebuffer)(GLint, GLint, GLint, GLint, GLint,      \
                                                           GLint, GLint, GLint, GLbitfield,        \
                                                           GLenum);                                \
            decl_kind void(GLAPIENTRY * glGetFramebufferAttachmentParameteriv)(GLenum, GLenum,     \
                                                                               GLenum, GLint*);    \
            decl_kind void(GLAPIENTRY * glUniform1f)(GLint, GLfloat);                              \
            decl_kind void(GLAPIENTRY * glUniform2f)(GLint, GLfloat, GLfloat);                     \
            decl_kind void(GLAPIENTRY * glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);            \
            decl_kind void(GLAPIENTRY * glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);   \
            decl_kind void(GLAPIENTRY * glUniform1i)(GLint, GLint);                                \
            decl_kind void(GLAPIENTRY * glUniformMatrix2fv)(GLint, GLsizei, GLboolean,             \
                                                            const GLfloat*);                       \
            decl_kind void(GLAPIENTRY * glUniformMatrix3fv)(GLint, GLsizei, GLboolean,             \
                                                            const GLfloat*);                       \
            decl_kind void(GLAPIENTRY * glUniformMatrix4fv)(GLint, GLsizei, GLboolean,             \
                                                            const GLfloat*);                       \
            decl_kind void(GLAPIENTRY * glGetTranslatedShaderSourceANGLE)(GLuint, GLsizei,         \
                                                                          GLsizei*,                \
                                                                          GLchar* source);         \
            decl_kind void(GLAPIENTRY * glDrawBuffers)(GLsizei, const GLenum*);                    \
            decl_kind void(GLAPIENTRY * glGenRenderbuffers)(GLsizei, GLuint*);                     \
            decl_kind void(GLAPIENTRY * glBindRenderbuffer)(GLenum, GLuint);                       \
            decl_kind void(GLAPIENTRY * glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei);  \
            decl_kind void(GLAPIENTRY * glFramebufferRenderbuffer)(GLenum, GLenum, GLenum,         \
                                                                   GLuint);                        \
        }                                                                                          \
    }

AK_GL_DECL_FUNCS(extern)
