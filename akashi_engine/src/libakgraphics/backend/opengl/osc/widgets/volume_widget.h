#pragma once

#include "./widget.h"

namespace akashi {
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class VolumeWidget final : public BaseWidget {
              public:
                struct Context;

              public:
                explicit VolumeWidget(OSCRenderContext& render_ctx, const BoundingBox& bbox);
                virtual ~VolumeWidget();

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool on_mouse_event(OSCRenderContext& render_ctx,
                                            const OSCMouseEvent& event) override;

              private:
                void emit_volume_changed(OSCRenderContext& render_ctx, const OSCMouseEvent& event);

              private:
                VolumeWidget::Context* m_ctx = nullptr;
            };
        }

    }
}
