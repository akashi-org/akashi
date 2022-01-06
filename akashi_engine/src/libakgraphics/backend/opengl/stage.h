#pragma once

#include <unordered_map>
#include <string>
#include <vector>

namespace akashi {
    namespace core {
        struct FrameContext;
        struct LayerContext;
    }
    namespace graphics {

        class OGLRenderContext;
        struct RenderParams;
        struct FBInfo;
        class Actor;

        struct StageAux;

        class Stage final {
            using uuid_t = std::string;
            using atom_uuid_t = std::string;

          public:
            explicit Stage() = default;
            virtual ~Stage() = default;

            bool create(const OGLRenderContext& ctx);
            bool render(OGLRenderContext& ctx, const RenderParams& params,
                        const core::FrameContext& frame_ctx);

            bool encode_render(OGLRenderContext& ctx, const core::FrameContext& frame_ctx);

            bool destroy(const OGLRenderContext& ctx);

          private:
            void init_gl();
            void init_renderer(const FBInfo& info, const core::FrameContext* frame_ctx = nullptr);
            bool render_layers(OGLRenderContext& ctx, const core::FrameContext& frame_ctx);
            bool add_layer(OGLRenderContext& ctx, const core::LayerContext& layer_ctx);

          private:
            std::vector<Actor*> m_actors;
            std::unordered_map<uuid_t, Actor*> m_actor_map;
            atom_uuid_t m_current_atom_uuid;
            StageAux* m_aux = nullptr;
        };
    }
}
