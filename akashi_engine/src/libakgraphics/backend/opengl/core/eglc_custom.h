#pragma once

#include "./glc.h"

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define AK_EGL_CUSTOM_LOADER 1;

#define AK_EGL_VOID ;

#define AK_EGL_DECL_FUNCS(decl_kind)                                                               \
    namespace akashi {                                                                             \
        namespace graphics {                                                                       \
                                                                                                   \
            decl_kind EGLint(EGLAPIENTRY* eglGetError)(void);                                      \
            decl_kind EGLDisplay(EGLAPIENTRY* eglGetCurrentDisplay)(void);                         \
            decl_kind EGLContext(EGLAPIENTRY* eglGetCurrentContext)(void);                         \
            decl_kind EGLImageKHR(EGLAPIENTRY* eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx,  \
                                                                  EGLenum target,                  \
                                                                  EGLClientBuffer buffer,          \
                                                                  const EGLint* attrib_list);      \
            decl_kind EGLBoolean(EGLAPIENTRY* eglDestroyImageKHR)(EGLDisplay dpy,                  \
                                                                  EGLImageKHR image);              \
            decl_kind void(EGLAPIENTRY * glEGLImageTargetTexture2DOES)(GLenum, GLeglImageOES);     \
        }                                                                                          \
    }

AK_EGL_DECL_FUNCS(extern)

namespace akashi {
    namespace graphics {

        struct EGLGetProcAddress;

        bool load_egl_functions(const EGLGetProcAddress& get_proc_address);

    }
}
