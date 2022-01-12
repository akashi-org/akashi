#include "./play_btn.h"

#include "../osc_render_context.h"
#include "../../../../osc.h"

#include "../common/text_label.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace graphics::osc {

        struct PlayBtn::Context {
            core::owned_ptr<osc::TextLabel> m_text_label = nullptr;
            state::PlayState cur_play_state = state::PlayState::PAUSED;
        };

        PlayBtn::PlayBtn(OSCRenderContext& render_ctx, const BoundingBox& bbox)
            : BaseWidget(render_ctx, bbox) {
            m_ctx = new PlayBtn::Context;

            osc::TextLabel::Params text_params;
            m_ctx->cur_play_state = state::PlayState::PAUSED;
            text_params.text = "\uf04b"; // \uf04c

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

        PlayBtn::~PlayBtn() {
            if (m_ctx) {
                delete m_ctx;
            }
        }

        bool PlayBtn::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            auto new_play_state = render_ctx.play_state();
            if (new_play_state == m_ctx->cur_play_state) {
                return false;
            }

            m_ctx->cur_play_state = new_play_state;

            auto text_params = m_ctx->m_text_label->obj_params();
            if (new_play_state == state::PlayState::PLAYING) {
                text_params.text = "\uf04c";
            } else {
                text_params.text = "\uf04b";
            }
            m_ctx->m_text_label->update_obj_params(text_params);

            return true;
        }

        bool PlayBtn::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            return m_ctx->m_text_label->render(render_ctx, params);
        }

        bool PlayBtn::on_mouse_event(OSCRenderContext& render_ctx, const OSCMouseEvent& event) {
            if (event.kind == OSCMouseEventKind::PRESS && event.btn == OSCMouseButton::LEFT) {
                render_ctx.emit_play_btn_clicked();
                return true;
            }

            return false;
        }

    }
}
