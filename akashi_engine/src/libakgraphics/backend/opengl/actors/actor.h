#pragma once

#include <libakcore/element.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;
        class FBO;
        class Camera;

        class Actor {
          public:
            virtual ~Actor() = default;

            virtual bool create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) = 0;

            virtual bool render(OGLRenderContext& ctx, const core::Rational& pts,
                                const Camera& camera) = 0;

            virtual bool destroy(const OGLRenderContext& ctx) = 0;

            virtual const core::LayerContext& get_layer_ctx(void) const { return m_layer_ctx; };

            virtual void update_layer(const core::LayerContext& layer_ctx) {
                m_layer_ctx = layer_ctx;
            };

            virtual void set_fbo(const core::borrowed_ptr<FBO>& /*fbo_ptr*/){};

          protected:
            core::LayerContext m_layer_ctx;
            core::LayerType m_layer_type;
        };

        Actor* create_actor(OGLRenderContext& ctx, const core::LayerContext& layer_ctx);

    }

}
