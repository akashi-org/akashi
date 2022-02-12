#pragma once

#include <libakcore/element.h>
#include "../widgets/widget.h"

namespace akashi {
    namespace graphics {

        class OSCRenderContext;
        struct RenderParams;

        namespace osc {

            class VolumeGauge final {
              public:
                struct Context;
                struct Params : BoundingBox {
                    double value = 0.0; // [0.0, 1.0]
                    double border_width = 0.0;
                    double border_radius = 0.0;
                    std::string color;
                    std::string border_color;
                    std::string inactive_color = "#ffffff00";
                };

              public:
                explicit VolumeGauge(const VolumeGauge::Params& init_params);
                virtual ~VolumeGauge();

                void update_obj_params(const VolumeGauge::Params& obj_params);

                const VolumeGauge::Params& obj_params() const { return m_obj_params; }

                bool render(OSCRenderContext& render_ctx, const RenderParams& params);

              private:
                bool load_border_pass();

                bool load_content_pass();

                bool render_border(OSCRenderContext& render_ctx, const RenderParams& params);

                bool render_content(OSCRenderContext& render_ctx, const RenderParams& params);

                void update_content_model_mat();

              private:
                VolumeGauge::Context* m_ctx = nullptr;
                VolumeGauge::Params m_obj_params;
            };

        }

    }
}
