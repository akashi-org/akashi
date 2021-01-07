#pragma once

#include <glm/glm.hpp>

namespace akashi {
    namespace core {
        struct LayerContext;
    }
    namespace graphics {

        struct GLRenderContext;
        struct GLTextureData;
        void update_translate(const GLRenderContext& ctx, const core::LayerContext& layer_ctx,
                              glm::mat4& new_mvp);

        void update_scale(const GLRenderContext& ctx, const GLTextureData& tex, glm::mat4& new_mvp,
                          const double scale_ratio = 1.0);

        // void update_mvp(GLRenderContext& ctx, const GLRenderPass& pass, glm::mat4& new_mvp);

    }
}
