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

            bool create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx);

            bool render(OGLRenderContext& ctx, const core::Rational& pts) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(OGLRenderContext& ctx);

            bool load_texture(OGLRenderContext& ctx);

            void update_model_mat();

          private:
            TextActor::Pass* m_pass = nullptr;
        };
    }

}