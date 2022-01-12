#include "./osc_root.h"
#include "./osc_render_context.h"

#include "./widgets/widget.h"
#include "./widgets/play_btn.h"
#include "./widgets/volume_widget.h"
#include "./widgets/timecode.h"
#include "./widgets/fseek_btn.h"
#include "./widgets/zoom_btn.h"
#include "./widgets/seek_widget/seek_widget.h"
#include "./common/bg_rect.h"

#include "../../../item.h"
#include "../../../osc.h"

#include "../core/glc.h"
#include "../core/shader.h"
#include "../meshes/rect.h"

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <vector>

namespace akashi {
    namespace graphics {

        struct OSCRoot::Context {
            std::vector<core::owned_ptr<osc::BaseWidget>> m_widgets;
            core::owned_ptr<osc::BGRect> m_rect;

            // constant params
            std::string root_bg_color = "#000000c8";
        };
        static void init_render(const RenderParams& params, const OSCRoot::Context& ctx) {
            glViewport(0.0, 0.0, params.screen_width, params.screen_height);
            glScissor(0.0, 0.0, params.screen_width, params.screen_height);
            glEnable(GL_SCISSOR_TEST);

            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            glDisable(GL_SCISSOR_TEST);

            glDisable(GL_MULTISAMPLE);
        }

        OSCRoot::OSCRoot(OSCEventCallback evt_cb, const RenderParams& params,
                         core::borrowed_ptr<state::AKState> state) {
            m_ctx = new OSCRoot::Context;
            init_render(params, *m_ctx);

            m_render_ctx = core::make_owned<OSCRenderContext>(evt_cb, params, state);

            auto pw = params.screen_width;
            auto ph = params.screen_height;

            m_ctx->m_widgets.push_back(core::make_owned<osc::PlayBtn>(
                *m_render_ctx, osc::BoundingBox{.cx = int(pw * 0.04),
                                                .cy = int(ph * 0.88),
                                                .w = int(pw * 0.028 * 0.9),
                                                .h = int(ph * 0.10 * 0.9)}));

            m_ctx->m_widgets.push_back(core::make_owned<osc::Timecode>(
                *m_render_ctx, osc::BoundingBox{.cx = int(pw * 0.51),
                                                .cy = int(ph * 0.885),
                                                .w = int(pw * 0.65),
                                                .h = int(ph * 0.10)}));

            m_ctx->m_widgets.push_back(core::make_owned<osc::ZoomBtn>(
                *m_render_ctx, osc::BoundingBox{.cx = int(pw * 0.96),
                                                .cy = int(ph * 0.88),
                                                .w = int(pw * 0.028),
                                                .h = int(ph * 0.10)}));

            m_ctx->m_widgets.push_back(core::make_owned<osc::VolumeWidget>(
                *m_render_ctx, osc::BoundingBox{.cx = int(pw * 0.12),
                                                .cy = int(ph * 0.88),
                                                .w = int(pw * 0.09),
                                                .h = int(ph * 0.09)}));

            m_ctx->m_widgets.push_back(core::make_owned<osc::FrameSeekBtn>(
                *m_render_ctx,
                osc::BoundingBox{.cx = int(pw * (1 - 0.028)),
                                 .cy = int(ph * 0.47),
                                 .w = int(pw * 0.028 * 0.8),
                                 .h = int(ph * 0.10 * 0.8)},
                osc::FrameSeekBtn::Params{.nframes_per_seek = 1}));

            m_ctx->m_widgets.push_back(core::make_owned<osc::FrameSeekBtn>(
                *m_render_ctx,
                osc::BoundingBox{.cx = int(pw * 0.028),
                                 .cy = int(ph * 0.47),
                                 .w = int(pw * 0.028 * 0.8),
                                 .h = int(ph * 0.10 * 0.8)},
                osc::FrameSeekBtn::Params{.nframes_per_seek = -1, .forward_seek = false}));

            m_ctx->m_widgets.push_back(core::make_owned<osc::SeekWidget>(
                *m_render_ctx, osc::BoundingBox{.cx = int(pw * 0.5),
                                                .cy = int(ph * 0.45),
                                                .w = int(pw * 1.0),
                                                .h = int(ph * 0.4)}));

            osc::BGRect::Params rect_params;
            rect_params.cx = pw * 0.5;
            rect_params.cy = ph * 0.5;
            rect_params.w = pw;
            rect_params.h = ph;
            rect_params.radius = 20.0;
            rect_params.color = m_ctx->root_bg_color;
            // rect_params.color = "#ff0000";
            m_ctx->m_rect = core::make_owned<osc::BGRect>(rect_params);
        }

        OSCRoot::~OSCRoot() {
            if (m_ctx) {
                delete m_ctx;
            }
            m_ctx = nullptr;
        }

        bool OSCRoot::update(const RenderParams& params) {
            // [TODO] impl fbo for skip rendering
            bool need_render = false;
            for (auto&& widget : m_ctx->m_widgets) {
                if (widget->update(*m_render_ctx, params)) {
                    need_render = true;
                }
            }
            // return need_render;
            return true;
        }

        bool OSCRoot::render(const RenderParams& params) {
            if (!m_ctx) {
                return false;
            }

            init_render(params, *m_ctx);

            glDisable(GL_BLEND);
            m_ctx->m_rect->render(*m_render_ctx, params);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            for (auto iter = m_ctx->m_widgets.rbegin(), end = m_ctx->m_widgets.rend(); iter != end;
                 ++iter) {
                auto& widget = *iter;
                widget->render(*m_render_ctx, params);
            }

            return true;
        }

        bool OSCRoot::on_mouse_event(const OSCMouseEvent& event) {
            for (auto&& widget : m_ctx->m_widgets) {
                switch (event.kind) {
                    case OSCMouseEventKind::PRESS: {
                        if (widget->has_collision(event.pos[0], event.pos[1])) {
                            if (widget->on_mouse_event(*m_render_ctx, event)) {
                                return true;
                            }
                        }
                        break;
                    }

                    case OSCMouseEventKind::MOVE: {
                        if (!widget->detect_mouse_move()) {
                            break;
                        }
                        if (widget->detect_grab_mouse_move() ||
                            widget->has_collision(event.pos[0], event.pos[1])) {
                            if (widget->on_mouse_event(*m_render_ctx, event)) {
                                return true;
                            }
                        }
                        break;
                    }

                    case OSCMouseEventKind::HOLD: {
                        if (!widget->detect_mouse_hold()) {
                            break;
                        }
                        if (widget->detect_grab_mouse_move() ||
                            widget->has_collision(event.pos[0], event.pos[1])) {
                            if (widget->on_mouse_event(*m_render_ctx, event)) {
                                return true;
                            }
                        }
                        break;
                    }

                    case OSCMouseEventKind::RELEASE: {
                        if (widget->on_mouse_event(*m_render_ctx, event)) {
                            return true;
                        }
                        break;
                    }

                    default: {
                        break;
                    }
                }
            }
            return false;
        }

        bool OSCRoot::on_time_event(const OSCTimeEvent& event) {
            bool should_update = false;
            for (auto&& widget : m_ctx->m_widgets) {
                if (widget->on_time_event(*m_render_ctx, event)) {
                    should_update = true;
                }
            }
            return should_update;
        }

    }
}
