#pragma once

#include <libakcore/memory.h>
#include "../core/glc.h"

namespace akashi {
    namespace buffer {
        class HWFrame;
    }
    namespace graphics {

        class OGLRenderContext;

        class HWEncodeFBO {
          public:
            explicit HWEncodeFBO(){};
            virtual ~HWEncodeFBO(){};

            virtual void render(OGLRenderContext& render_ctx, GLuint rgba_tex,
                                core::borrowed_ptr<buffer::HWFrame> hwframe) = 0;
        };

    }
}
