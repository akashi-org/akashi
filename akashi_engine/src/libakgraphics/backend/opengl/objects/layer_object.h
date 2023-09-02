#pragma once

#include "../core/glc.h"
#include "./layer_commons.h"

#include <libakcore/rational.h>
#include <libakcore/memory.h>

#include <libakcore/element.h>

namespace akashi {
    namespace core {
        struct LayerContext;
    }
    namespace graphics {

        class OGLRenderContext;
        class FBO;
        class Camera;

        class LayerObject {
          public:
            explicit LayerObject() = default;
            virtual ~LayerObject() = default;

            bool create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx,
                        const core::Rational& pts);
            bool update(OGLRenderContext& ctx, const core::LayerContext& layer_ctx,
                        const core::Rational& pts);
            bool render(OGLRenderContext& ctx, const core::Rational& pts, const Camera& camera);
            bool destroy(const OGLRenderContext& ctx);

            void set_fbo(const core::borrowed_ptr<FBO>& fbo_ptr);

            bool is_program_ready() const { return m_is_program_ready; }
            bool is_buffers_ready() const { return m_is_buffers_ready; }

            bool is_unit() const { return m_layer_type == layer::LayerType::UNIT; }
            bool can_display() const { return m_can_display; }
            const std::string& layer_uuid() const { return m_safe_ctx.uuid; }
            const layer::SafeLayerContext& safe_ctx() const { return m_safe_ctx; }

          private:
            void set_buffers_ready();
            bool load_shaders(const core::LayerContext& layer_ctx);
            bool load_transform(const core::LayerContext& layer_ctx);
            bool render_inner(OGLRenderContext& ctx, const core::Rational& pts,
                              const Camera& camera);

          private:
            GLuint m_prog;
            layer::Transform m_transform;

            layer::Texture m_texture;
            layer::Mesh m_mesh;

            core::Rational m_current_pts = core::Rational(-1, 1);
            core::borrowed_ptr<FBO> m_fbo{nullptr};

            bool m_is_program_ready = false;
            bool m_is_buffers_ready = false;

            bool m_has_video_decode_method = false;

            bool m_can_display = false;
            layer::SafeLayerContext m_safe_ctx;
            layer::LayerType m_layer_type = layer::LayerType::LENGTH;
            std::array<long, 2> m_unit_fb_size = {0, 0};
        };

    }

}
