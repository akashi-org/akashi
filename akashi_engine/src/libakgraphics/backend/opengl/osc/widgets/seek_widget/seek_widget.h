#pragma once

#include "../widget.h"

#include <cstddef>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OSCRenderContext;

        namespace osc {

            class SeekWidget final : public BaseWidget {
              public:
                struct Context;
                enum SeekTimeResult {
                    OK = 0,
                    NOT_PLAYABLE = -1,
                    OUT_OF_BAND = -2,
                    DUP_TIME = -3,
                    ZERO_DURATION = -4,
                };

              public:
                explicit SeekWidget(OSCRenderContext& render_ctx, const BoundingBox& bbox);
                virtual ~SeekWidget();

                virtual bool update(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool render(OSCRenderContext& render_ctx,
                                    const RenderParams& params) override;

                virtual bool on_mouse_event(OSCRenderContext& render_ctx,
                                            const OSCMouseEvent& event) override;

                virtual bool on_time_event(OSCRenderContext& render_ctx,
                                           const OSCTimeEvent& event) override;

              private:
                bool seek_value_for_mouse(size_t* seek_value, OSCRenderContext& render_ctx,
                                          const OSCMouseEvent& event);

                SeekTimeResult seek_time_for_mouse(core::Rational* seek_time,
                                                   const size_t seek_value,
                                                   OSCRenderContext& render_ctx,
                                                   const OSCMouseEvent& event);

                void seek_value_for_time(size_t* seek_value, core::Rational* seek_offset,
                                         size_t* seek_band_index,
                                         const core::Rational& current_time);

                void update_zoom_level(OSCRenderContext& render_ctx);

              private:
                SeekWidget::Context* m_ctx = nullptr;
            };
        }

    }
}
