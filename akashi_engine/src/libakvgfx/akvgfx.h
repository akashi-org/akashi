#pragma once

#include <libakcore/class.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace core {
        struct LayerContext;
    }
    namespace vgfx {

        struct SurfaceInfo;
        struct SurfaceEntry;

        class Surface {
          public:
            explicit Surface(){};
            virtual ~Surface(){};

            virtual const SurfaceInfo& info() const = 0;
            virtual uint8_t* buffer() const = 0;
        };

        core::owned_ptr<vgfx::Surface> create_surface(const SurfaceEntry& entry,
                                                      const core::LayerContext& layer_ctx);

    }
}
