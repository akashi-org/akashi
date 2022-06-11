#pragma once

#include "../core/glc.h"
#include "./actor.h"

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;

        class UnitActor : public Actor {
            struct Pass;

          public:
            explicit UnitActor() = default;
            virtual ~UnitActor() = default;
            UnitActor(UnitActor&&) = default;

            bool create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) override;

            bool render(OGLRenderContext& ctx, const core::Rational& pts,
                        const Camera& camera) override;

            bool destroy(const OGLRenderContext& ctx) override;

            void set_fbo(const core::borrowed_ptr<FBO>& fbo_ptr) override;

          private:
            bool load_pass(const OGLRenderContext& ctx);

          private:
            UnitActor::Pass* m_pass = nullptr;
        };
    }

}
