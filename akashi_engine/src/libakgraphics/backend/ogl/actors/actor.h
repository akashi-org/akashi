#pragma once

#include <libakcore/rational.h>

namespace akashi {
    namespace graphics {

        class OGLRenderContext;

        class Actor {
          public:
            virtual ~Actor() = default;

            virtual bool render(const OGLRenderContext& ctx, const core::Rational& pts) = 0;

            virtual bool destroy(const OGLRenderContext& ctx) = 0;
        };
    }

}
