#pragma once

#include "../../widgets/widget.h"
#include <cstddef>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;
        struct RenderParams;

        namespace osc {

            class SeekRail final : public BaseWidget {
              public:
                struct Context;
                struct Params {
                    long seek_area_width = 0;
                };

              public:
                explicit SeekRail(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                                  const SeekRail::Params& init_params);
                virtual ~SeekRail();

                void update_obj_params(const SeekRail::Params& obj_params);

                const SeekRail::Params& obj_params() const { return m_obj_params; }

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

              private:
                bool load_pass();

              private:
                SeekRail::Context* m_ctx = nullptr;
                SeekRail::Params m_obj_params;
            };

        }

    }
}
