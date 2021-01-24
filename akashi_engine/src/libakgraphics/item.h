#pragma once

#include <functional>

namespace akashi {
    namespace graphics {

        struct GetProcAddress {
            std::function<void*(void* ctx, const char* name)> func;
        };

        struct EGLGetProcAddress {
            std::function<void*(void* ctx, const char* name)> func;
        };

        struct RenderParams {
            unsigned int default_fb;
            int screen_width;
            int screen_height;
        };

    }

}
