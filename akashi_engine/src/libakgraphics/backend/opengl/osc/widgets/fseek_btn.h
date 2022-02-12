#pragma once

#include "./widget.h"

#include <libakcore/perf.h>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class FrameSeekBtn final : public BaseWidget {
              public:
                struct Context;
                struct Params {
                    int nframes_per_seek = 1;
                    bool forward_seek = true;
                };

              public:
                explicit FrameSeekBtn(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                                      const FrameSeekBtn::Params& obj_params);
                virtual ~FrameSeekBtn();

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool on_mouse_event(OSCRenderContext& render_ctx,
                                            const OSCMouseEvent& event) override;

              private:
                FrameSeekBtn::Context* m_ctx = nullptr;
                FrameSeekBtn::Params m_obj_params;
                core::Timer m_timer;
            };
        }

    }
}
