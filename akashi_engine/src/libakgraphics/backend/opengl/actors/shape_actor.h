#pragma once

#include "../core/glc.h"
#include "./actor.h"

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;

        class ShapeActor : public Actor {
            struct Pass;

          public:
            explicit ShapeActor() = default;
            virtual ~ShapeActor() = default;
            ShapeActor(ShapeActor&&) = default;

            bool create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) override;

            bool render(OGLRenderContext& ctx, const core::Rational& pts) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(const OGLRenderContext& ctx);

            bool load_mesh(const OGLRenderContext& ctx);

          private:
            ShapeActor::Pass* m_pass = nullptr;
        };
    }

}
