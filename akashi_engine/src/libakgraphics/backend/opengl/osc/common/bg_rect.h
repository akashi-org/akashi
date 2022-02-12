#pragma once

#include "../widgets/widget.h"

#include <string>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;
        struct RenderParams;

        namespace osc {

            class BGRect final {
              public:
                struct Context;
                struct Params : BoundingBox {
                    double radius = 0.0;
                    std::string color;
                };

              public:
                explicit BGRect(const BGRect::Params& init_params);

                virtual ~BGRect();

                void update_obj_params(const BGRect::Params& obj_params);

                const BGRect::Params& obj_params() const { return m_obj_params; }

                bool render(OSCRenderContext& render_ctx, const RenderParams& params);

              private:
                bool load_pass();

                void update_content_model_mat();

              private:
                BGRect::Context* m_ctx = nullptr;
                BGRect::Params m_obj_params;
            };

        }

    }
}
