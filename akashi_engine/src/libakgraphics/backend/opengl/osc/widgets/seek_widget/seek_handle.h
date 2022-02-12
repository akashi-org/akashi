#pragma once

#include "../widget.h"

#include <glm/glm.hpp>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class SeekHandle final : public BaseWidget {
              public:
                struct Context;
                struct Params {
                    long seek_area_width = 0;
                    double seek_value = 0; // [0.0, 1.0]
                };

              public:
                explicit SeekHandle(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                                    const SeekHandle::Params& params);
                virtual ~SeekHandle();

                void update_obj_params(const SeekHandle::Params& obj_params);

                const SeekHandle::Params& obj_params() const { return m_obj_params; }

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

              private:
                bool load_line_pass();

                bool render_line_pass(OSCRenderContext& render_ctx, const RenderParams& params);

                bool load_knob_pass(OSCRenderContext& render_ctx);

                bool render_knob_pass(OSCRenderContext& render_ctx, const RenderParams& params);

                void update_line_transform(glm::mat4& model_mat);

                void update_knob_transform(glm::mat4& model_mat);

              private:
                SeekHandle::Context* m_ctx = nullptr;
                SeekHandle::Params m_obj_params;
            };
        }

    }
}
