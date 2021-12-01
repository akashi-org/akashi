#pragma once

namespace akashi {
    namespace core {
        class Rational;
    }
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
