#pragma once

namespace akashi {
    namespace graphics {

        struct GetProcAddress;

        bool load_gl_functions(const GetProcAddress& get_proc_address);

        // void load_egl_functions(const EGLGetProcAddress& egl_get_proc_address,
        //                         GLRenderContext& ctx);

    }
}
