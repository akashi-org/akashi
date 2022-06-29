#pragma once

#include <libakcore/class.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace buffer {

        class GFXHWContext {
            AK_FORBID_COPY(GFXHWContext)
          public:
            explicit GFXHWContext(){};
            virtual ~GFXHWContext(){};
        };

        struct HWFrameInfo;

        class HWFrame {
            AK_FORBID_COPY(HWFrame)
          public:
            explicit HWFrame(){};
            virtual ~HWFrame(){};

            virtual HWFrameInfo hw_info() const = 0;

            virtual void set_gfx_hwctx(core::owned_ptr<GFXHWContext> gfx_hwctx) = 0;
        };

    }
}
