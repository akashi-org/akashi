#include "./fseek_btn.h"

#include "../osc_render_context.h"
#include "../../../../osc.h"

#include "../common/text_label.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace graphics::osc {

        struct FrameSeekBtn::Context {
            core::owned_ptr<osc::TextLabel> m_text_label = nullptr;
        };

        FrameSeekBtn::FrameSeekBtn(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                                   const FrameSeekBtn::Params& obj_params)
            : BaseWidget(render_ctx, bbox), m_obj_params(obj_params) {
            m_ctx = new FrameSeekBtn::Context;

            osc::TextLabel::Params text_params;
            if (m_obj_params.forward_seek) {
                // text_params.text = "\uf054"; // big
                text_params.text = "\uf105"; // small
            } else {
                // text_params.text = "\uf053"; // big
                text_params.text = "\uf104"; // small
            }

            text_params.style.font_path = render_ctx.default_font_path();
            if (std::getenv("AK_ASSET_DIR")) {
                text_params.style.font_path =
                    std::string(std::getenv("AK_ASSET_DIR")) +
                    "/fonts/fontawesome-free-5.12.1-desktop/otfs/Font Awesome 5 Free-Solid-900.otf";
            }

            text_params.style.fg_color = "#ffffff";
            text_params.style.fg_size = 22;

            text_params.cx = bbox.cx;
            text_params.cy = bbox.cy;
            text_params.w = bbox.w;
            text_params.h = bbox.h;

            m_ctx->m_text_label = core::make_owned<osc::TextLabel>(text_params);
        }

        FrameSeekBtn::~FrameSeekBtn() {
            if (m_ctx) {
                delete m_ctx;
            }
        }

        bool FrameSeekBtn::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            return false;
        }

        bool FrameSeekBtn::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            return m_ctx->m_text_label->render(render_ctx, params);
        }

        bool FrameSeekBtn::on_mouse_event(OSCRenderContext& render_ctx,
                                          const OSCMouseEvent& event) {
            auto seek_coef = render_ctx.seek_base().sec_per_unit_coef;
            switch (event.kind) {
                case OSCMouseEventKind::PRESS: {
                    if (event.btn == OSCMouseButton::LEFT) {
                        render_ctx.emit_frame_seek(m_obj_params.nframes_per_seek * seek_coef);
                        m_timer.reset();
                        m_timer.start();
                        m_detect_mouse_hold = true;
                        return true;
                    }
                    break;
                }

                case OSCMouseEventKind::HOLD: {
                    if (event.btn == OSCMouseButton::LEFT && m_timer.current_time() > 0.2) {
                        render_ctx.emit_frame_seek(m_obj_params.nframes_per_seek * seek_coef);
                        m_timer.reset();
                        m_timer.start();
                        return true;
                    }
                    break;
                }

                case OSCMouseEventKind::RELEASE: {
                    m_detect_mouse_hold = false;
                    m_timer.reset();
                    return false;
                }

                default: {
                    break;
                }
            }

            return false;
        }

    }
}
