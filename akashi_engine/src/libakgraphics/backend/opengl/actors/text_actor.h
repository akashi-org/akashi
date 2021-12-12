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

            bool render(OGLRenderContext& ctx, const core::Rational& pts) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(OGLRenderContext& ctx);

            bool load_label_pass(OGLRenderContext& ctx);

            bool load_border_pass(OGLRenderContext& ctx);

            bool load_texture(OGLRenderContext& ctx);

            bool load_label_texture(TextActor::Pass& pass, const std::string& src);

            bool render_pass(const TextActor::Pass& pass, OGLRenderContext& ctx,
                             const core::Rational& pts);

            void update_model_mat(TextActor::Pass& pass);

          private:
            TextActor::Pass* m_pass = nullptr;
            TextActor::Pass* m_lb_pass = nullptr;
            TextActor::Pass* m_border_pass = nullptr;
        };
    }

}
