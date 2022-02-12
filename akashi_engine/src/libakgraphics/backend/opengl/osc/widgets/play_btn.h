#pragma once

#include "./widget.h"

namespace akashi {
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class PlayBtn final : public BaseWidget {
              public:
                struct Context;

              public:
                explicit PlayBtn(OSCRenderContext& render_ctx, const BoundingBox& bbox);
                virtual ~PlayBtn();

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool on_mouse_event(OSCRenderContext& render_ctx,
                                            const OSCMouseEvent& event) override;

              private:
                PlayBtn::Context* m_ctx = nullptr;
            };
        }

    }
}
