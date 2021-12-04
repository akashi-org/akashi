#pragma once

#include "../core/glc.h"
#include "./actor.h"

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;

        class EffectActor : public Actor {
            struct Pass;

          public:
            explicit EffectActor() = default;
            virtual ~EffectActor() = default;
            EffectActor(EffectActor&&) = default;

            bool create(const OGLRenderContext& ctx, const core::LayerContext& layer_ctx);

            bool render(OGLRenderContext& ctx, const core::Rational& pts) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(const OGLRenderContext& ctx);

          private:
            EffectActor::Pass* m_pass = nullptr;
        };
    }

}
