#pragma once

#include "./actor.h"

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;

        class TextActor : public Actor {
            struct Pass;

          public:
            explicit TextActor() = default;
            virtual ~TextActor() = default;
            TextActor(TextActor&&) = default;

            bool create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) override;

            bool render(OGLRenderContext& ctx, const core::Rational& pts,
                        const Camera& camera) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(OGLRenderContext& ctx);

            bool load_texture(OGLRenderContext& ctx);

            bool render_pass(const TextActor::Pass& pass, OGLRenderContext& ctx,
                             const core::Rational& pts, const Camera& camera);

          private:
            TextActor::Pass* m_pass = nullptr;
        };
    }

}
