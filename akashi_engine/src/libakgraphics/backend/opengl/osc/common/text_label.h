#pragma once

#include <libakcore/element.h>
#include "../widgets/widget.h"

#include <glm/glm.hpp>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;
        struct RenderParams;
        struct OGLTexture;

        namespace osc {

            class TextLabel final {
              public:
                struct Context;
                struct Params : core::TextLayerContext, BoundingBox {
                    std::array<int32_t, 4> pad = {0, 0, 0, 0};
                    int32_t line_span = 0;
                };

              public:
                explicit TextLabel(const TextLabel::Params& init_params);
                virtual ~TextLabel();

                void update_obj_params(const TextLabel::Params& obj_params);

                void update_transform(const glm::mat4& model_mat);

                const TextLabel::Params& obj_params() const { return m_obj_params; }

                bool render(OSCRenderContext& render_ctx, const RenderParams& params);

              private:
                bool load_pass();

                bool create_ogl_texture(OGLTexture& tex);

                bool load_texture();

                bool update_texture();

                bool load_mesh();

              private:
                TextLabel::Context* m_ctx = nullptr;
                TextLabel::Params m_obj_params;
                bool m_dirty = false;
            };

        }

    }
}
