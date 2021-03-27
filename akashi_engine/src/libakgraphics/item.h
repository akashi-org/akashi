#pragma once

#include <functional>
#include <cstdint>

namespace akashi {
    namespace graphics {

        using FrameBufferHandle = uint32_t;

        struct GetProcAddress {
            std::function<void*(void* ctx, const char* name)> func;
        };

        struct EGLGetProcAddress {
            std::function<void*(void* ctx, const char* name)> func;
        };

        struct RenderParams {
            FrameBufferHandle default_fb;
            int screen_width;
            int screen_height;
        };

        struct EncodeRenderParams {
            /* fields to be filled by the callee */
            uint8_t* buffer = nullptr;
            int width = -1;
            int height = -1;
        };

    }

}
