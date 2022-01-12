#pragma once

#include "./widget.h"

#include <libakcore/perf.h>

namespace akashi {
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class ZoomBtn final : public BaseWidget {
              public:
                struct Context;

              public:
                explicit ZoomBtn(OSCRenderContext& render_ctx, const BoundingBox& bbox);
                virtual ~ZoomBtn();

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool on_mouse_event(OSCRenderContext& render_ctx,
                                            const OSCMouseEvent& event) override;

              private:
                ZoomBtn::Context* m_ctx = nullptr;
            };
        }

    }
}
