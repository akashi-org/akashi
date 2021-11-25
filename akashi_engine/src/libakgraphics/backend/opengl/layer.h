#pragma once

#include "./gl.h"
#include "./objects/quad/layer_quad.h"
#include "./objects/quad/video_quad.h"

#include <libakcodec/akcodec.h>

#include <libakcore/common.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <stdexcept>

struct SDL_Surface;

namespace akashi {
    namespace core {
        class Rational;
        struct LayerContext;
        enum class LayerType;
    }
    namespace buffer {
        class AVBufferData;
    }
    namespace graphics {

        class GLGraphicsContext;
        struct GLRenderContext;
        class PlayerContext;
        class LayerTarget {
          public:
            virtual ~LayerTarget(void){};

            virtual bool render(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                                const GLRenderContext& ctx, const core::Rational& pts) = 0;
            virtual bool destroy(const GLRenderContext& ctx) = 0;

            virtual const core::LayerContext& get_layer_ctx(void) const { return m_layer_ctx; };

            virtual void update_layer(core::LayerContext layer_ctx) { m_layer_ctx = layer_ctx; };

          protected:
            akashi::core::LayerContext m_layer_ctx;
            akashi::core::LayerType m_layer_type;
        };

        class VideoLayerTarget final : public LayerTarget {
          public:
            explicit VideoLayerTarget(void) = default;
            virtual ~VideoLayerTarget(void) = default;
            VideoLayerTarget(VideoLayerTarget&&) = default;

            bool create(const GLRenderContext& ctx, akashi::core::LayerContext layer_ctx);
            bool render(core::borrowed_ptr<GLGraphicsContext> glx_ctx, const GLRenderContext& ctx,
                        const akashi::core::Rational& pts) override;
            bool destroy(const GLRenderContext& ctx) override;

          private:
            VideoQuadObject m_quad_obj;
            core::Rational m_current_pts = core::Rational(-1, 1);
        };

        class TextLayerTarget final : public LayerTarget {
          public:
            explicit TextLayerTarget(void) = default;
            virtual ~TextLayerTarget(void) = default;
            TextLayerTarget(TextLayerTarget&&) = default;

            bool create(const GLRenderContext& ctx, core::LayerContext layer_ctx);
            bool render(core::borrowed_ptr<GLGraphicsContext> glx_ctx, const GLRenderContext& ctx,
                        const core::Rational& pts) override;
            bool destroy(const GLRenderContext& ctx) override;

          private:
            bool load_mesh(const GLRenderContext& ctx, LayerQuadMesh& mesh,
                           core::LayerContext& layer_ctx) const;
            bool load_texture(const GLRenderContext& ctx, GLTextureData& tex,
                              core::LayerContext& layer_ctx) const;

          private:
            LayerQuadObject m_quad_obj;
        };

        class ImageLayerTarget final : public LayerTarget {
          public:
            explicit ImageLayerTarget(void) = default;
            virtual ~ImageLayerTarget(void) = default;
            ImageLayerTarget(ImageLayerTarget&&) = default;

            bool create(const GLRenderContext& ctx, core::LayerContext layer_ctx);
            bool render(core::borrowed_ptr<GLGraphicsContext> glx_ctx, const GLRenderContext& ctx,
                        const core::Rational& pts) override;
            bool destroy(const GLRenderContext& ctx) override;

          private:
            bool load_mesh(const GLRenderContext& ctx, LayerQuadMesh& mesh,
                           core::LayerContext& layer_ctx);
            bool load_texture(const GLRenderContext& ctx, GLTextureData& tex,
                              core::LayerContext& layer_ctx);

          private:
            LayerQuadObject m_quad_obj;
            std::vector<SDL_Surface*> m_surfaces;
        };

        class EffectLayerTarget final : public LayerTarget {
          public:
            explicit EffectLayerTarget(void) = default;
            virtual ~EffectLayerTarget(void) = default;
            EffectLayerTarget(EffectLayerTarget&&) = default;

            bool create(const GLRenderContext& ctx, core::LayerContext layer_ctx);
            bool render(core::borrowed_ptr<GLGraphicsContext> glx_ctx, const GLRenderContext& ctx,
                        const core::Rational& pts) override;
            bool destroy(const GLRenderContext& ctx) override;

          private:
            LayerQuadObject m_quad_obj;
        };

    }
}
