#include "./seek_widget.h"

#include "./seek_handle.h"
#include "./seek_ruler.h"
#include "./seek_rail.h"

#include "../../../core/glc.h"
#include "../../osc_render_context.h"
#include "../../../../../osc.h"

#include "../../common/volume_gauge.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>
#include <libakcore/error.h>
#include <libakcore/rational.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics::osc {

        struct SeekWidget::Context {
            core::owned_ptr<osc::VolumeGauge> seek_area = nullptr;
            core::owned_ptr<osc::SeekHandle> seek_handle = nullptr;
            core::owned_ptr<osc::SeekRuler> seek_ruler = nullptr;

            size_t seek_value = 0; // [0, unit]

            size_t seek_unit = 360;
            size_t seek_ruler_unit = 40;
            core::Rational sec_per_unit{1, 24}; // 1/fps

            size_t zoom_level = 1;
            bool m_is_second_mode = true;

            core::Rational seek_offset{0, 1};

            double seek_area_ratio = 0; // [0.0, 1.0]
            size_t max_seekable_value = 0;

            size_t max_band_index = 0;
            size_t seek_band_index = 0;

            bool seek_moved_triggered = false;

            bool should_calculate_ruler_label_content = false;
        };

        SeekWidget::SeekWidget(OSCRenderContext& render_ctx, const BoundingBox& bbox)
            : BaseWidget(render_ctx, bbox) {
            m_ctx = new SeekWidget::Context;
            this->update_zoom_level(render_ctx);

            osc::VolumeGauge::Params gauge_params;

            // gauge_params.cx = bbox.cx - int(std::max(2.0, bbox.w * 0.005));
            gauge_params.cx = bbox.cx + 1.0;
            gauge_params.cy = bbox.cy + (bbox.h * 0.5 * (0.5 - 0.34));
            gauge_params.w = int(bbox.w * 0.9);
            gauge_params.h = int(bbox.h * (0.34 + 0.5));
            gauge_params.value = 1.0;
            gauge_params.border_width = 1.0;
            gauge_params.border_radius = 0.0;
            // gauge_params.color = "#0872a8ee"; blue
            gauge_params.color = "#909090ee";
            gauge_params.border_color = "#ffffff00";
            gauge_params.inactive_color = "#000000ee";

            m_ctx->seek_area = core::make_owned<osc::VolumeGauge>(gauge_params);

            osc::SeekHandle::Params handle_params;
            handle_params.seek_area_width = m_bbox.w * 0.9;
            handle_params.seek_value = 0.0;

            m_ctx->seek_handle = core::make_owned<osc::SeekHandle>(
                render_ctx,
                osc::BoundingBox{.cx = m_calc_bbox.left +
                                       (int)((m_bbox.w - handle_params.seek_area_width) / 2) + 2,
                                 .cy = bbox.cy,
                                 .w = int(std::max(2.0, bbox.w * 0.005)),
                                 .h = int(m_bbox.h * 1.0)},
                handle_params);

            osc::SeekRuler::Params ruler_params;
            ruler_params.seek_ruler = m_ctx->seek_ruler_unit;
            ruler_params.seek_area_width = handle_params.seek_area_width;

            ruler_params.label.style.font_path = render_ctx.default_font_path();
            if (std::getenv("AK_ASSET_DIR")) {
                ruler_params.label.style.font_path =
                    std::string(std::getenv("AK_ASSET_DIR")) +
                    "/fonts/liberation-fonts-ttf-2.1.5/LiberationSans-Bold.ttf";
            }

            ruler_params.label.text_align = core::TextAlign::CENTER;
            ruler_params.label.style.fg_color = "#ffffff";
            ruler_params.label.style.fg_size = 18;

            m_ctx->seek_ruler = core::make_owned<osc::SeekRuler>(
                render_ctx,
                osc::BoundingBox{.cx = m_calc_bbox.left +
                                       (int)((m_bbox.w - handle_params.seek_area_width) / 2),
                                 .cy = int(m_bbox.cy - (bbox.h * 0.34) - (m_bbox.h * 0.12 / 2.0)),
                                 .w = int(std::max(1.0, bbox.w * 0.0025)),
                                 .h = int(m_bbox.h * 0.12)},
                ruler_params);
        }

        SeekWidget::~SeekWidget() {
            if (m_ctx) {
                delete m_ctx;
            }
        }

        bool SeekWidget::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            bool mode_changed = render_ctx.is_second_mode() != m_ctx->m_is_second_mode;
            if (mode_changed) {
                m_ctx->m_is_second_mode = render_ctx.is_second_mode();
            }

            if (mode_changed || render_ctx.is_second_mode() != m_ctx->m_is_second_mode ||
                m_ctx->zoom_level != render_ctx.zoom_level()) {
                this->update_zoom_level(render_ctx);
            }

            int should_update = 0;

            auto handle_params = m_ctx->seek_handle->obj_params();
            auto seek_ratio = core::Rational(m_ctx->seek_value, m_ctx->seek_unit).to_decimal();
            handle_params.seek_value = std::max(0.0, std::min(1.0, seek_ratio));
            m_ctx->seek_handle->update_obj_params(handle_params);
            should_update += m_ctx->seek_handle->update(render_ctx, params);

            auto gauge_params = m_ctx->seek_area->obj_params();
            gauge_params.value = m_ctx->seek_area_ratio;
            m_ctx->seek_area->update_obj_params(gauge_params);
            // should we impl update()?
            // should_update += m_ctx->seek_area->update(render_ctx, params);

            if (m_ctx->should_calculate_ruler_label_content) {
                auto ruler_params = m_ctx->seek_ruler->obj_params();

                auto sec_per_label =
                    core::Rational(m_ctx->seek_unit, ruler_params.label_texts.size()) *
                    m_ctx->sec_per_unit;

                for (size_t i = 0; i < ruler_params.label_texts.size(); i++) {
                    auto label_sec = m_ctx->seek_offset + (core::Rational(i, 1) * sec_per_label);
                    size_t label_frame_num = (label_sec * render_ctx.fps()).to_decimal();
                    ruler_params.label_texts[i] = label_frame_num;
                }
                m_ctx->seek_ruler->update_obj_params(ruler_params);
                if (mode_changed) {
                    m_ctx->seek_ruler->set_label_dirty(true);
                }
                should_update += m_ctx->seek_ruler->update(render_ctx, params);

                m_ctx->should_calculate_ruler_label_content = false;
            }

            return should_update > 0;
        }

        bool SeekWidget::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            CHECK_AK_ERROR2(m_ctx->seek_area->render(render_ctx, params));
            CHECK_AK_ERROR2(m_ctx->seek_ruler->render(render_ctx, params));
            CHECK_AK_ERROR2(m_ctx->seek_handle->render(render_ctx, params));
            return true;
        }

        bool SeekWidget::on_mouse_event(OSCRenderContext& render_ctx, const OSCMouseEvent& event) {
            switch (event.kind) {
                case OSCMouseEventKind::PRESS: {
                    if (event.btn != OSCMouseButton::LEFT) {
                        return false;
                    } else {
                        size_t seek_value;
                        if (!this->seek_value_for_mouse(&seek_value, render_ctx, event)) {
                            return false;
                        }

                        core::Rational seek_time;
                        if (this->seek_time_for_mouse(&seek_time, seek_value, render_ctx, event) <
                            0) {
                            return true;
                        }

                        m_detect_mouse_move = true;
                        m_detect_grab_mouse_move = true;
                        m_ctx->seek_value = seek_value;
                        render_ctx.emit_seekbar_pressed(seek_time);
                        return true;
                    }
                }

                case OSCMouseEventKind::MOVE: {
                    if (m_detect_mouse_move) {
                        m_ctx->seek_moved_triggered = true;

                        size_t seek_value;
                        if (!this->seek_value_for_mouse(&seek_value, render_ctx, event)) {
                            return false;
                        }

                        core::Rational seek_time;
                        if (this->seek_time_for_mouse(&seek_time, seek_value, render_ctx, event) <
                            0) {
                            return true;
                        }

                        m_detect_mouse_hold = true;
                        m_ctx->seek_value = seek_value;
                        render_ctx.emit_seekbar_moved(seek_time);
                    }
                    return true;
                }

                case OSCMouseEventKind::HOLD: {
                    size_t seek_value;
                    if (!this->seek_value_for_mouse(&seek_value, render_ctx, event)) {
                        return false;
                    }

                    if (seek_value > m_ctx->seek_unit - 1) {
                        seek_value = m_ctx->seek_unit - 1;
                    }

                    core::Rational seek_time;
                    if (auto r =
                            this->seek_time_for_mouse(&seek_time, seek_value, render_ctx, event);
                        r < 0) {
                        return true;
                        // [TODO impl this later!
                        // if (r != SeekTimeResult::NOT_PLAYABLE) {
                        //     return true;
                        // }

                        // auto time_ratio = seek_time / render_ctx.duration();
                        // if (time_ratio > core::Rational(1l)) {
                        //     auto max_v = render_ctx.duration() / m_ctx->seek_unit;
                        // } else {
                        // }
                    }

                    m_ctx->seek_value = seek_value;
                    render_ctx.emit_seekbar_moved(seek_time);
                    return true;
                }

                case OSCMouseEventKind::RELEASE: {
                    if (m_detect_mouse_move && m_ctx->seek_moved_triggered) {
                        render_ctx.emit_seekbar_released(0);
                    }
                    m_detect_mouse_move = false;
                    m_detect_grab_mouse_move = false;
                    m_detect_mouse_hold = false;
                    m_ctx->seek_moved_triggered = false;
                    return false;
                }

                default: {
                    return false;
                }
            }

            return false;
        }

        bool SeekWidget::on_time_event(OSCRenderContext& render_ctx, const OSCTimeEvent& event) {
            core::Rational current_time{0, 1};
            core::Rational duration{0, 1};
            switch (event.kind) {
                case OSCTimeEventKind::CURRENT_TIME: {
                    duration = render_ctx.duration();
                    current_time = event.current_time;
                    break;
                }
                case OSCTimeEventKind::DURATION: {
                    duration = event.duration;
                    current_time = render_ctx.current_time();
                    if (duration >= core::Rational(0l)) {
                        this->update_zoom_level(render_ctx);
                    }
                    break;
                }
                default: {
                    return false;
                }
            }

            if (duration <= core::Rational(0l)) {
                return false;
            }

            auto time_ratio = current_time / duration;
            if (time_ratio > core::Rational(1l) || time_ratio < core::Rational(0l)) {
                return false;
            }

            size_t seek_value = 0;
            size_t seek_band_index = 0;
            this->seek_value_for_time(&seek_value, &m_ctx->seek_offset, &seek_band_index,
                                      current_time);

            if (seek_band_index != m_ctx->seek_band_index) {
                m_ctx->should_calculate_ruler_label_content = true;
            }
            m_ctx->seek_band_index = seek_band_index;

            if (m_ctx->seek_band_index < m_ctx->max_band_index) {
                m_ctx->seek_area_ratio = 1.0;
            } else if (m_ctx->seek_band_index == m_ctx->max_band_index) {
                m_ctx->seek_area_ratio =
                    core::Rational(m_ctx->max_seekable_value, m_ctx->seek_unit).to_decimal();
            } else {
                m_ctx->seek_area_ratio = 0.0;
            }

            if (seek_value == m_ctx->seek_value) {
                return false;
            } else {
                m_ctx->seek_value = seek_value;

                return true;
            }
        }

        bool SeekWidget::seek_value_for_mouse(size_t* seek_value, OSCRenderContext& /*render_ctx*/,
                                              const OSCMouseEvent& event) {
            auto handle_bbox = m_ctx->seek_handle->bounding_box();
            auto handle_area_width = m_ctx->seek_handle->obj_params().seek_area_width;
            auto raw_seek_value = (event.pos[0] - (1.0 * handle_bbox.cx)) / handle_area_width;
            if (raw_seek_value < 0) {
                return false;
            }
            // seek_value = std::max(0.0, std::min(1.0, seek_value));
            *seek_value = int(raw_seek_value * m_ctx->seek_unit);

            return true;
        }

        SeekWidget::SeekTimeResult SeekWidget::seek_time_for_mouse(core::Rational* seek_time,
                                                                   const size_t seek_value,
                                                                   OSCRenderContext& render_ctx,
                                                                   const OSCMouseEvent& /*event*/) {
            if (render_ctx.duration() <= core::Rational(0l)) {
                *seek_time = 0;
                return SeekTimeResult::ZERO_DURATION;
            }

            *seek_time = (m_ctx->sec_per_unit * seek_value) + m_ctx->seek_offset;
            auto time_ratio = *seek_time / render_ctx.duration();

            if (time_ratio > core::Rational(1l) || time_ratio < core::Rational(0l)) {
                return SeekTimeResult::NOT_PLAYABLE;
            }

            if (seek_value > m_ctx->seek_unit - 1) {
                return SeekTimeResult::OUT_OF_BAND;
            }

            if (seek_value == m_ctx->seek_value) {
                return SeekTimeResult::DUP_TIME;
            }

            return SeekTimeResult::OK;
        }

        void SeekWidget::seek_value_for_time(size_t* seek_value, core::Rational* seek_offset,
                                             size_t* seek_band_index,
                                             const core::Rational& current_time) {
            auto sec_per_band = m_ctx->sec_per_unit * core::Rational(m_ctx->seek_unit, 1);
            auto band_index = core::Rational((current_time / sec_per_band).to_decimal(), 1);
            auto seek_ratio = ((current_time - (sec_per_band * band_index)) / sec_per_band);

            *seek_value = (seek_ratio * m_ctx->seek_unit).to_decimal();
            *seek_offset = band_index * sec_per_band;
            *seek_band_index = band_index.to_decimal();
        }

        void SeekWidget::update_zoom_level(OSCRenderContext& render_ctx) {
            render_ctx.update_second_seek_base();

            m_ctx->zoom_level = render_ctx.zoom_level();

            m_ctx->should_calculate_ruler_label_content = true;

            auto seek_base = render_ctx.seek_base();

            m_ctx->seek_unit = seek_base.unit;
            m_ctx->seek_ruler_unit = seek_base.ruler_unit;
            m_ctx->sec_per_unit = core::Rational(seek_base.sec_per_unit_coef, 1) *
                                  (core::Rational(1l) / render_ctx.fps());

            this->seek_value_for_time(&m_ctx->seek_value, &m_ctx->seek_offset,
                                      &m_ctx->seek_band_index, render_ctx.current_time());

            core::Rational d;
            this->seek_value_for_time(&m_ctx->max_seekable_value, &d, &m_ctx->max_band_index,
                                      render_ctx.duration());

            if (m_ctx->seek_band_index < m_ctx->max_band_index) {
                m_ctx->seek_area_ratio = 1.0;
            } else if (m_ctx->seek_band_index == m_ctx->max_band_index) {
                m_ctx->seek_area_ratio =
                    core::Rational(m_ctx->max_seekable_value, m_ctx->seek_unit).to_decimal();
            } else {
                m_ctx->seek_area_ratio = 0.0;
            }
        }

    }
}
