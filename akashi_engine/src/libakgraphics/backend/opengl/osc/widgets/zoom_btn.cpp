#include "./zoom_btn.h"

#include "../osc_render_context.h"
#include "../../../../osc.h"

#include "../common/text_label.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace graphics::osc {

        struct ZoomBtn::Context {
            core::owned_ptr<osc::TextLabel> m_text_label = nullptr;
            std::string icon_text = "\uf00e";
            bool dirty = false;
        };

        ZoomBtn::ZoomBtn(OSCRenderContext& render_ctx, const BoundingBox& bbox)
            : BaseWidget(render_ctx, bbox) {
            m_ctx = new ZoomBtn::Context;

            osc::TextLabel::Params text_params;
            text_params.text = "\uf00e";

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

        ZoomBtn::~ZoomBtn() {
            if (m_ctx) {
                delete m_ctx;
            }
        }

        bool ZoomBtn::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            if (!m_ctx->dirty) {
                return false;
            }

            auto text_params = m_ctx->m_text_label->obj_params();
            text_params.text = m_ctx->icon_text;
            m_ctx->m_text_label->update_obj_params(text_params);

            m_ctx->dirty = false;

            return true;
        }

        bool ZoomBtn::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            return m_ctx->m_text_label->render(render_ctx, params);
        }

        bool ZoomBtn::on_mouse_event(OSCRenderContext& render_ctx, const OSCMouseEvent& event) {
            switch (event.kind) {
                case OSCMouseEventKind::PRESS: {
                    if (event.btn == OSCMouseButton::LEFT) {
                        if (render_ctx.is_second_mode()) {
                            render_ctx.set_second_mode(false);
                            m_ctx->icon_text = "\uf010";
                        } else {
                            render_ctx.set_second_mode(true);
                            m_ctx->icon_text = "\uf00e";
                        }
                        m_ctx->dirty = true;
                        render_ctx.emit_update();
                        return true;
                    }
                    break;
                }

                case OSCMouseEventKind::RELEASE: {
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
