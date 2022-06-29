#pragma once

#include <libakcore/memory.h>

#include <functional>
#include <cstdint>

namespace akashi {
    namespace buffer {
        class HWFrame;
    }
    namespace graphics {

        using FrameBufferHandle = uint32_t;

        struct GetProcAddress {
            void* (*func)(const char*) = nullptr;
        };

        struct EGLGetProcAddress {
            void* (*func)(const char*) = nullptr;
        };

        struct RenderParams {
            FrameBufferHandle default_fb;
            int screen_width;
            int screen_height;
            std::array<int, 2> mouse_pos = {-1, -1};
        };

        struct EncodeRenderParams {
            /* fields to be filled by the callee */
            uint8_t* buffer = nullptr;
            int width = -1;
            int height = -1;
            core::borrowed_ptr<buffer::HWFrame> hwframe;
        };

    }

}
