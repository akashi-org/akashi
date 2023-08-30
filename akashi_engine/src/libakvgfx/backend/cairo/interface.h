#pragma once

#include "../../akvgfx.h"
#include "../../item.h"
#include <cairo/cairo.h>

namespace akashi {
    namespace vgfx {

        class CairoSurface : public Surface {
          public:
            explicit CairoSurface(const SurfaceKind& kind, cairo_surface_t* cairo_surface);
            virtual ~CairoSurface();

            virtual const SurfaceInfo& info() const override;

            virtual uint8_t* buffer() const override;

          private:
            void load_info(const SurfaceKind& kind);

          private:
            SurfaceInfo m_info;
            cairo_surface_t* m_cairo_surface = nullptr;
            uint8_t* m_buffer = nullptr;
        };

        core::owned_ptr<vgfx::Surface> cairo_create_surface(const SurfaceEntry& entry,
                                                            const core::LayerContext& layer_ctx);

    }
}
