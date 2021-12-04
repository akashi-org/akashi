#pragma once

#include "./actor.h"

struct SDL_Surface;

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;

        class ImageActor : public Actor {
            struct Pass;

          public:
            explicit ImageActor() = default;
            virtual ~ImageActor() = default;
            ImageActor(ImageActor&&) = default;

            bool create(const OGLRenderContext& ctx, const core::LayerContext& layer_ctx);

            bool render(OGLRenderContext& ctx, const core::Rational& pts) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool load_pass(const OGLRenderContext& ctx);

            bool load_texture(const OGLRenderContext& ctx);

            void update_model_mat();

          private:
            ImageActor::Pass* m_pass = nullptr;
            std::vector<SDL_Surface*> m_surfaces;
        };
    }

}
