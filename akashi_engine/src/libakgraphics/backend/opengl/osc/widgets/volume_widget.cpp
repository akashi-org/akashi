#include "./volume_widget.h"

#include "../osc_render_context.h"
#include "../../../../osc.h"

#include "../common/volume_gauge.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace graphics::osc {

        struct VolumeWidget::Context {
            core::owned_ptr<osc::VolumeGauge> volume_gauge = nullptr;
        };

        VolumeWidget::VolumeWidget(OSCRenderContext& render_ctx, const BoundingBox& bbox)
            : BaseWidget(render_ctx, bbox) {
            m_ctx = new VolumeWidget::Context;

            osc::VolumeGauge::Params gauge_params;

            m_collision_offset.left = int(bbox.w * 0.2);
            m_collision_offset.right = int(bbox.w * 0.2);
            m_collision_offset.top = int(bbox.h * 0.2);
            m_collision_offset.bottom = int(bbox.h * 0.2);

            gauge_params.cx = bbox.cx;
            gauge_params.cy = bbox.cy;
            gauge_params.w = bbox.w;
            gauge_params.h = bbox.h;
            gauge_params.value = render_ctx.gain();
            gauge_params.border_width = 1.0;
            gauge_params.border_radius = 0.0;
            // gauge_params.color = "#00ff55"; // green
            gauge_params.color = "#00dbac"; // neon light green
            gauge_params.border_color = "#ffffff";

            m_ctx->volume_gauge = core::make_owned<osc::VolumeGauge>(gauge_params);
        }

        VolumeWidget::~VolumeWidget() {
            if (m_ctx) {
                delete m_ctx;
            }
        }

        bool VolumeWidget::update(OSCRenderContext& render_ctx, const RenderParams& /*params*/) {
            auto gauge_params = m_ctx->volume_gauge->obj_params();
            gauge_params.value = render_ctx.gain();
            m_ctx->volume_gauge->update_obj_params(gauge_params);
            return true;
        }

        bool VolumeWidget::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            return m_ctx->volume_gauge->render(render_ctx, params);
        }

        bool VolumeWidget::on_mouse_event(OSCRenderContext& render_ctx,
                                          const OSCMouseEvent& event) {
            switch (event.kind) {
                case OSCMouseEventKind::PRESS: {
                    if (event.btn == OSCMouseButton::LEFT) {
                        m_detect_mouse_move = true;
                        m_detect_grab_mouse_move = true;
                        this->emit_volume_changed(render_ctx, event);
                        return true;
                    }
                    return false;
                }

                case OSCMouseEventKind::MOVE: {
                    if (m_detect_mouse_move && event.btn == OSCMouseButton::LEFT) {
                        this->emit_volume_changed(render_ctx, event);
                    }
                    return false;
                }

                case OSCMouseEventKind::RELEASE: {
                    m_detect_mouse_move = false;
                    m_detect_grab_mouse_move = false;
                    return false;
                }

                default: {
                    return false;
                }
            }
        }

        void VolumeWidget::emit_volume_changed(OSCRenderContext& render_ctx,
                                               const OSCMouseEvent& event) {
            auto gain = (event.pos[0] - (1.0 * m_calc_bbox.left)) / m_bbox.w;
            gain = std::max(0.0, std::min(1.0, gain));
            render_ctx.emit_volume_changed(gain);
        }

    }
}
