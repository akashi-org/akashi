#include "./interface.h"
#include "./utils.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace vgfx {

        CairoSurface::CairoSurface(const SurfaceKind& kind, cairo_surface_t* cairo_surface)
            : Surface(), m_cairo_surface(cairo_surface) {
            cairo_surface_flush(m_cairo_surface);
            m_buffer = cairo_image_surface_get_data(m_cairo_surface);
            this->load_info(kind);
        }

        CairoSurface::~CairoSurface() {
            // No need to free m_buffer
            if (m_cairo_surface) {
                cairo_surface_destroy(m_cairo_surface);
            }
        }

        const SurfaceInfo& CairoSurface::info() const { return m_info; };

        uint8_t* CairoSurface::buffer() const { return m_buffer; }

        void CairoSurface::load_info(const SurfaceKind& kind) {
            m_info.width = cairo_image_surface_get_width(m_cairo_surface);
            m_info.height = cairo_image_surface_get_height(m_cairo_surface);
            m_info.stride = cairo_image_surface_get_stride(m_cairo_surface);
            m_info.format = from_cairo_format(cairo_image_surface_get_format(m_cairo_surface));
            m_info.kind = kind;

            // [TODO] support big-endian system
            m_info.format_swap = true;
        }

    }
}
