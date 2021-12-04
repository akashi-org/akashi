#include "../../../item.h"

#include "./eglc.h"

#include <libakcore/logger.h>

#include <string>

using namespace akashi::core;

#ifdef AK_EGL_CUSTOM_LOADER

#define DEF_FN_CHECK(proc_addr, func_name)                                                         \
    do {                                                                                           \
        auto func = proc_addr.func(#func_name);                                                    \
        if (!func) {                                                                               \
            AKLOG_ERROR("Could not load EGL function: {}", func_name);                             \
            return false;                                                                          \
        }                                                                                          \
        akashi::graphics::func_name =                                                              \
            reinterpret_cast<decltype(akashi::graphics::func_name)>(func);                         \
    } while (0)

AK_EGL_DECL_FUNCS(AK_EGL_VOID)

namespace akashi {
    namespace graphics {

        bool load_egl_functions(const EGLGetProcAddress& get_proc_address) {
            DEF_FN_CHECK(get_proc_address, eglGetError);
            DEF_FN_CHECK(get_proc_address, eglGetCurrentDisplay);
            DEF_FN_CHECK(get_proc_address, eglGetCurrentContext);
            DEF_FN_CHECK(get_proc_address, eglCreateImageKHR);
            DEF_FN_CHECK(get_proc_address, eglDestroyImageKHR);
            DEF_FN_CHECK(get_proc_address, glEGLImageTargetTexture2DOES);
            return true;
        }

    }
}

#endif
