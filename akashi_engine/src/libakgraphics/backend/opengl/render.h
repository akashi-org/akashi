#pragma once

#include <libakcore/memory.h>

#include <unordered_map>
#include <string>
#include <vector>

#include <GL/gl.h>

namespace akashi {
    namespace core {
        struct FrameContext;
        struct LayerContext;
    }
    namespace graphics {

        struct GLRenderContext;
        class LayerTarget;
        struct RenderParams;
        class GLGraphicsContext;
        class RenderScene final {
            using uuid_t = std::string;
            using atom_uuid_t = std::string;

            struct RenderInitParams {
                GLuint fb;
                int width;
                int height;
            };

          public:
            explicit RenderScene(void) = default;
            virtual ~RenderScene(void) = default;

            bool create(const GLRenderContext& ctx);
            bool render(core::borrowed_ptr<GLGraphicsContext> glx_ctx, const GLRenderContext& ctx,
                        const RenderParams& params, const core::FrameContext& frame_ctx);

            bool encode_render(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                               const GLRenderContext& ctx, const core::FrameContext& frame_ctx);

            bool destroy(const GLRenderContext& ctx);

          private:
            bool render_init(const GLRenderContext& ctx, const RenderInitParams& params) const;
            bool add_layer(const GLRenderContext& ctx, const core::LayerContext& layer_ctx);
            bool render_layer(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                              const GLRenderContext& ctx, const core::FrameContext& frame_ctx);

          private:
            std::vector<LayerTarget*> m_targets;
            std::unordered_map<uuid_t, LayerTarget*> m_target_map;
            atom_uuid_t m_current_atom_uuid;
        };
    }
}
