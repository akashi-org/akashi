#include "./timecode.h"

#include "../osc_render_context.h"
#include "../../../../osc.h"

#include "../common/text_label.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>
#include <libakcore/time.h>

#include <sstream>
#include <iomanip>

namespace akashi {
    namespace graphics::osc {

        struct Timecode::Context {
            core::owned_ptr<osc::TextLabel> text_label = nullptr;
            core::Rational cur_frame_time{1, 24}; // 1 / fps
            long cur_time_msec = 0;
            long cur_duration_msec = 0;
            size_t cur_frame = 0;
            size_t cur_total_frames = 2000;

            std::string str_cur_time_msec;
            std::string str_cur_duration_msec;
            std::string str_cur_frame;
            std::string str_cur_total_frames;
        };
        Timecode::Timecode(OSCRenderContext& render_ctx, const BoundingBox& bbox)
            : BaseWidget(render_ctx, bbox) {
            m_ctx = new Timecode::Context;

            m_ctx->cur_frame_time = core::Rational(1l) / render_ctx.fps();

            m_ctx->str_cur_time_msec = core::to_time_string(m_ctx->cur_time_msec);
            m_ctx->str_cur_duration_msec = core::to_time_string(m_ctx->cur_duration_msec);
            m_ctx->str_cur_frame = this->formatted_frame_count(m_ctx->cur_frame);
            m_ctx->str_cur_total_frames = this->formatted_frame_count(m_ctx->cur_total_frames);

            osc::TextLabel::Params text_params;
            text_params.text = this->construct_time_string();
            text_params.style.font_path = render_ctx.default_font_path();
            if (std::getenv("AK_ASSET_DIR")) {
                text_params.style.font_path =
                    std::string(std::getenv("AK_ASSET_DIR")) +
                    "/fonts/liberation-fonts-ttf-2.1.5/LiberationSans-Bold.ttf";
            }

            text_params.style.fg_color = "#ffffff";
            text_params.style.fg_size = 20;
            text_params.cx = bbox.cx;
            text_params.cy = bbox.cy;
            text_params.w = bbox.w;
            text_params.h = bbox.h;
            text_params.size_pref = osc::TextLabel::MeshSizePref::FIXED_WIDTH_TEX;

            m_ctx->text_label = core::make_owned<osc::TextLabel>(text_params);
        }

        Timecode::~Timecode() {
            if (m_ctx) {
                delete m_ctx;
            }
        }

        bool Timecode::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            auto text_params = m_ctx->text_label->obj_params();
            text_params.text = this->construct_time_string();
            m_ctx->text_label->update_obj_params(text_params);
            return true;
        }

        bool Timecode::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            return m_ctx->text_label->render(render_ctx, params);
        }

        bool Timecode::on_time_event(OSCRenderContext& render_ctx, const OSCTimeEvent& event) {
            switch (event.kind) {
                case OSCTimeEventKind::CURRENT_TIME: {
                    auto cur_time_msec =
                        static_cast<long>(event.current_time.to_decimal() * 1000.0);
                    if (cur_time_msec == m_ctx->cur_time_msec) {
                        return false;
                    } else {
                        m_ctx->cur_time_msec = cur_time_msec;
                        m_ctx->str_cur_time_msec = core::to_time_string(m_ctx->cur_time_msec);

                        m_ctx->cur_frame =
                            (event.current_time / m_ctx->cur_frame_time).to_decimal();
                        m_ctx->str_cur_frame = this->formatted_frame_count(m_ctx->cur_frame);

                        return true;
                    }
                }
                case OSCTimeEventKind::DURATION: {
                    auto cur_duration_msec =
                        static_cast<long>(event.duration.to_decimal() * 1000.0);
                    if (cur_duration_msec == m_ctx->cur_duration_msec) {
                        return false;
                    } else {
                        m_ctx->cur_duration_msec = cur_duration_msec;
                        m_ctx->str_cur_duration_msec =
                            core::to_time_string(m_ctx->cur_duration_msec);

                        m_ctx->cur_total_frames =
                            (event.duration / m_ctx->cur_frame_time).to_decimal();
                        m_ctx->str_cur_total_frames =
                            this->formatted_frame_count(m_ctx->cur_total_frames);

                        return true;
                    }
                }

                default: {
                    return false;
                }
            }
        }

        std::string Timecode::construct_time_string() {
            return m_ctx->str_cur_time_msec + " / " + m_ctx->str_cur_duration_msec + " ( " +
                   m_ctx->str_cur_frame + " / " + m_ctx->str_cur_total_frames + " )";
        }

        std::string Timecode::formatted_frame_count(long count, int pad_digit) {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(pad_digit) << count;
            return oss.str();
        }

    }
}
