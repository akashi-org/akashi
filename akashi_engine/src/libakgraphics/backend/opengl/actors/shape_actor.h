#pragma once

#include "../core/glc.h"
#include "./actor.h"

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace vgfx {
        class Surface;
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

            bool render(OGLRenderContext& ctx, const core::Rational& pts,
                        const Camera& camera) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(OGLRenderContext& ctx);

            bool load_mesh(OGLRenderContext& ctx);

            bool load_texture(const vgfx::Surface& surface);

          private:
            ShapeActor::Pass* m_pass = nullptr;
        };
    }

}
