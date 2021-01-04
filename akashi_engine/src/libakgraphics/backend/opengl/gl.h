#pragma once

#include <libakcore/element.h>

#include <string>
#include <vector>
#include <functional>

#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/glm.hpp>

#define PREFIXED_GLFUNC(func_name) akpriv##func_name

#define GET_GLFUNC(gl_render_ctx, func_name) gl_render_ctx.func.PREFIXED_GLFUNC(func_name)

namespace akashi {
    namespace graphics {

        class RenderScene;
        class FramebufferObject;
        class QuadPass;

        struct GLFunction {
            // >= opengl 3.0
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGenerateMipmap))(GLenum);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glBlendFunc))(GLenum, GLenum);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glViewport))(GLint, GLint, GLsizei, GLsizei);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glClear))(GLbitfield);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGenTextures))(GLsizei, GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteTextures))(GLsizei, const GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glClearColor))(GLclampf, GLclampf, GLclampf, GLclampf);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glEnable))(GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDisable))(GLenum);
            const GLubyte*(GLAPIENTRY* PREFIXED_GLFUNC(glGetString))(GLenum);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glBlendFuncSeparate))(GLenum, GLenum, GLenum,
            // GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glFlush))(void);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glFinish))(void);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glPixelStorei))(GLenum, GLint); void(GLAPIENTRY*
            // PREFIXED_GLFUNC(glTexImage1D))(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum,
            //                              const GLvoid*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glTexImage2D))(GLenum, GLint, GLint, GLsizei, GLsizei,
                                                            GLint, GLenum, GLenum, const GLvoid*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glTexSubImage2D))(GLenum, GLint, GLint, GLint, GLsizei,
                                                               GLsizei, GLenum, GLenum,
                                                               const GLvoid*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glTexParameteri))(GLenum, GLenum, GLint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetIntegerv))(GLenum, GLint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glReadPixels))(GLint, GLint, GLsizei, GLsizei,
            // GLenum, GLenum, GLvoid*); void(GLAPIENTRY* PREFIXED_GLFUNC(glReadBuffer))(GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDrawArrays))(GLenum, GLint, GLsizei);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDrawElements))(GLenum mode, GLsizei count,
                                                              GLenum type, const void* indices);

            // GLenum(GLAPIENTRY* PREFIXED_GLFUNC(glGetError))(void); void(GLAPIENTRY**
            // GetTexLevelParameteriv))(GLenum, GLint, GLenum, GLint*); void(GLAPIENTRY**
            // Scissor))(GLint, GLint, GLsizei, GLsizei);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glGenBuffers))(GLsizei, GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteBuffers))(GLsizei, const GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindBuffer))(GLenum, GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindBufferBase))(GLenum, GLuint, GLuint);
            GLvoid*(GLAPIENTRY* PREFIXED_GLFUNC(glMapBufferRange))(GLenum, GLintptr, GLsizeiptr,
                                                                   GLbitfield);
            GLboolean(GLAPIENTRY* PREFIXED_GLFUNC(glUnmapBuffer))(GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBufferData))(GLenum, intptr_t, const GLvoid*,
                                                            GLenum);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glBufferSubData))(GLenum, GLintptr, GLsizeiptr,
                                                               const GLvoid*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glActiveTexture))(GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindTexture))(GLenum, GLuint);
            int(GLAPIENTRY* PREFIXED_GLFUNC(glSwapInterval))(int);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glTexImage3D))(GLenum, GLint, GLenum, GLsizei, GLsizei,
                                                            GLsizei, GLint, GLenum, GLenum,
                                                            const GLvoid*);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glGenVertexArrays))(GLsizei, GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindVertexArray))(GLuint);
            GLint(GLAPIENTRY* PREFIXED_GLFUNC(glGetAttribLocation))(GLuint, const GLchar*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glEnableVertexAttribArray))(GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDisableVertexAttribArray))(GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glVertexAttribPointer))(GLuint, GLint, GLenum,
                                                                     GLboolean, GLsizei,
                                                                     const GLvoid*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteVertexArrays))(GLsizei, const GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUseProgram))(GLuint);
            GLint(GLAPIENTRY* PREFIXED_GLFUNC(glGetUniformLocation))(GLuint, const GLchar*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glCompileShader))(GLuint);
            GLuint(GLAPIENTRY* PREFIXED_GLFUNC(glCreateProgram))(void);
            GLuint(GLAPIENTRY* PREFIXED_GLFUNC(glCreateShader))(GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glShaderSource))(GLuint, GLsizei, const GLchar**,
                                                              const GLint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glLinkProgram))(GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glAttachShader))(GLuint, GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteShader))(GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteProgram))(GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetShaderInfoLog))(GLuint, GLsizei, GLsizei*,
                                                                  GLchar*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetShaderiv))(GLuint, GLenum, GLint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetProgramInfoLog))(GLuint, GLsizei, GLsizei*,
                                                                   GLchar*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetProgramiv))(GLenum, GLenum, GLint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glGetProgramBinary))(GLuint, GLsizei, GLsizei*,
            // GLenum*, void*); void(GLAPIENTRY* PREFIXED_GLFUNC(glProgramBinary))(GLuint, GLenum,
            // const void*, GLsizei);

            // void(GLAPIENTRY* PREFIXED_GLFUNC(glDispatchCompute))(GLuint, GLuint, GLuint);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glBindImageTexture))(GLuint, GLuint, GLint,
            // GLboolean, GLint, GLenum,
            //                                    GLenum);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glMemoryBarrier))(GLbitfield);

            const GLubyte*(GLAPIENTRY* PREFIXED_GLFUNC(glGetStringi))(GLenum, GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindAttribLocation))(GLuint, GLuint, const GLchar*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindFramebuffer))(GLenum, GLuint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGenFramebuffers))(GLsizei, GLuint*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteFramebuffers))(GLsizei, const GLuint*);
            GLenum(GLAPIENTRY* PREFIXED_GLFUNC(glCheckFramebufferStatus))(GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glFramebufferTexture2D))(GLenum, GLenum, GLenum,
                                                                      GLuint, GLint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glBlitFramebuffer))(GLint, GLint, GLint, GLint, GLint,
                                                                 GLint, GLint, GLint, GLbitfield,
                                                                 GLenum);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetFramebufferAttachmentParameteriv))(GLenum, GLenum,
                                                                                     GLenum,
                                                                                     GLint*);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniform1f))(GLint, GLfloat);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniform2f))(GLint, GLfloat, GLfloat);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniform3f))(GLint, GLfloat, GLfloat, GLfloat);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniform4f))(GLint, GLfloat, GLfloat, GLfloat,
                                                           GLfloat);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniform1i))(GLint, GLint);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniformMatrix2fv))(GLint, GLsizei, GLboolean,
                                                                  const GLfloat*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniformMatrix3fv))(GLint, GLsizei, GLboolean,
                                                                  const GLfloat*);
            void(GLAPIENTRY* PREFIXED_GLFUNC(glUniformMatrix4fv))(GLint, GLsizei, GLboolean,
                                                                  const GLfloat*);

            // void(GLAPIENTRY* PREFIXED_GLFUNC(glInvalidateTexImage))(GLuint, GLint);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glInvalidateFramebuffer))(GLenum, GLsizei, const
            // GLenum*);

            // GLsync(GLAPIENTRY* PREFIXED_GLFUNC(glFenceSync))(GLenum, GLbitfield);
            // GLenum(GLAPIENTRY* PREFIXED_GLFUNC(glClientWaitSync))(GLsync, GLbitfield, GLuint64);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteSync))(GLsync sync);

            // void(GLAPIENTRY* PREFIXED_GLFUNC(glBufferStorage))(GLenum, intptr_t, const GLvoid*,
            // GLenum);

            // void(GLAPIENTRY* PREFIXED_GLFUNC(glGenQueries))(GLsizei, GLuint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glDeleteQueries))(GLsizei, const GLuint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glBeginQuery))(GLenum, GLuint);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glEndQuery))(GLenum);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glQueryCounter))(GLuint, GLenum);
            // GLboolean(GLAPIENTRY* PREFIXED_GLFUNC(glIsQuery))(GLuint);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glGetQueryObjectiv))(GLuint, GLenum, GLint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glGetQueryObjecti64v))(GLuint, GLenum, GLint64*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glGetQueryObjectuiv))(GLuint, GLenum, GLuint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glGetQueryObjectui64v))(GLuint, GLenum, GLuint64*);

            // void(GLAPIENTRY* PREFIXED_GLFUNC(glVDPAUInitNV))(const GLvoid*, const GLvoid*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glVDPAUFiniNV))(void);
            // GLvdpauSurfaceNV(GLAPIENTRY*
            // PREFIXED_GLFUNC(glVDPAURegisterOutputSurfaceNV))(GLvoid*, GLenum, GLsizei,
            //                                                            const GLuint*);
            // GLvdpauSurfaceNV(GLAPIENTRY* PREFIXED_GLFUNC(glVDPAURegisterVideoSurfaceNV))(GLvoid*,
            // GLenum, GLsizei,
            //                                                           const GLuint*);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glVDPAUUnregisterSurfaceNV))(GLvdpauSurfaceNV);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glVDPAUSurfaceAccessNV))(GLvdpauSurfaceNV, GLenum);
            // void(GLAPIENTRY* PREFIXED_GLFUNC(glVDPAUMapSurfacesNV))(GLsizei, const
            // GLvdpauSurfaceNV*); void(GLAPIENTRY*
            // PREFIXED_GLFUNC(glVDPAUUnmapSurfacesNV))(GLsizei, const GLvdpauSurfaceNV*);

            // GLint(GLAPIENTRY* PREFIXED_GLFUNC(glGetVideoSync))(GLuint*);
            // GLint(GLAPIENTRY* PREFIXED_GLFUNC(glWaitVideoSync))(GLint, GLint, unsigned int*);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glGetTranslatedShaderSourceANGLE))(GLuint, GLsizei,
                                                                                GLsizei*,
                                                                                GLchar* source);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glDrawBuffers))(GLsizei, const GLenum*);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glGenRenderbuffers))(GLsizei, GLuint*);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glBindRenderbuffer))(GLenum, GLuint);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glRenderbufferStorage))(GLenum, GLenum, GLsizei,
                                                                     GLsizei);

            void(GLAPIENTRY* PREFIXED_GLFUNC(glFramebufferRenderbuffer))(GLenum, GLenum, GLenum,
                                                                         GLuint);
        };

        struct GLTextureData {
            int index = 0;
            GLuint buffer;
            void* image = nullptr;
            void* surface = nullptr;
            int width;  // includes stride
            int height; // includes stdide
            int effective_width;
            int effective_height;
            GLenum format = GL_RGBA;
            GLenum internal_format = GL_RGBA;
        };

        struct GLRenderContext {
            /** OpenGL version.
             * e.g. 210 for OpenGL 2.1
             */
            int version;

            /**
             * GLSL version.
             * e.g. 130 for GLSL 1.30
             */
            int glsl_version;

            /**
             * OpenGL extensions
             */
            std::string extensions;

            GLFunction func;

            // [TODO] unique_ptr?
            RenderScene* render_scene = nullptr;

            // [TODO] unique_ptr?
            FramebufferObject* fbo = nullptr;

            std::string default_font_path;
        };

    }
}
