#pragma once

#include "../../item.h"
#include <cairo/cairo.h>

namespace akashi {
    namespace vgfx {

        cairo_format_t to_cairo_format(const vgfx::SurfaceFormat& format);

        vgfx::SurfaceFormat from_cairo_format(const cairo_format_t& format);

    }
}
