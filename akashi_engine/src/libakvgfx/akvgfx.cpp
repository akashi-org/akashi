#include "./akvgfx.h"
#include "./backend/cairo/interface.h"

#include <libakcore/element.h>

namespace akashi {
    namespace vgfx {

        core::owned_ptr<vgfx::Surface> create_surface(const SurfaceEntry& entry,
                                                      const core::ShapeLayerContext& shape_ctx) {
            return cairo_create_surface(entry, shape_ctx);
        }

    }
}
