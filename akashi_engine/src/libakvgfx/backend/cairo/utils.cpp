#include "./utils.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace vgfx {

        cairo_format_t to_cairo_format(const vgfx::SurfaceFormat& format) {
            switch (format) {
                case vgfx::SurfaceFormat::ARGB32: {
                    return CAIRO_FORMAT_ARGB32;
                }
                case vgfx::SurfaceFormat::RGB24: {
                    return CAIRO_FORMAT_RGB24;
                }
                default: {
                    AKLOG_ERROR("Invalid format {} found", format);
                    return CAIRO_FORMAT_INVALID;
                }
            }
        }

        vgfx::SurfaceFormat from_cairo_format(const cairo_format_t& format) {
            switch (format) {
                case CAIRO_FORMAT_ARGB32: {
                    return vgfx::SurfaceFormat::ARGB32;
                }
                case CAIRO_FORMAT_RGB24: {
                    return vgfx::SurfaceFormat::RGB24;
                }
                default: {
                    AKLOG_ERROR("Invalid format {} found", format);
                    return vgfx::SurfaceFormat::NONE;
                }
            }
        }

    }
}
