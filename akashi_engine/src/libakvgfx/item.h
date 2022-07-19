#pragma once

#include <cstdint>

namespace akashi {
    namespace vgfx {

        enum class SurfaceKind { NONE = -1, RECT, CIRCLE, TRI, LINE, OTHER };

        enum class SurfaceFormat {
            NONE = -1,
            ARGB32 = 0,
            RGB24,
        };

        struct SurfaceInfo {
            int32_t width = 0;
            int32_t height = 0;
            int32_t stride = 0;
            SurfaceFormat format = SurfaceFormat::NONE;
            SurfaceKind kind = SurfaceKind::NONE;
            bool format_swap = true; // ex. RGB => BGR in a little-endian system
        };

        struct SurfaceEntry {
            int32_t width = 0;
            int32_t height = 0;
            SurfaceFormat format = SurfaceFormat::NONE;
            int32_t border_padv = 4;
        };

    }
}
