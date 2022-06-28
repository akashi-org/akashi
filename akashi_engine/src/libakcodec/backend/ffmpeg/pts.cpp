#include "./pts.h"
#include "./source.h"
#include "./utils.h"

#include <libakcore/rational.h>
#include <libakcore/logger.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}

using namespace akashi::core;

namespace akashi {
    namespace codec {

        akashi::core::Rational pts_to_rational(const int64_t pts, const AVRational& time_base) {
            return akashi::core::Rational(pts) * to_rational(time_base);
        }
        akashi::core::Rational rpts_to_pts(const akashi::core::Rational& rpts,
                                           const akashi::core::Rational& from,
                                           const akashi::core::Rational& start) {
            return rpts - (start - from);
        }

        PTSSet::PTSSet(const InputSource* input_src, const AVFrame* frame, const int stream_index) {
            m_input_src = input_src;
            m_stream_index = stream_index;

            const DecodeStream* dec_stream = &input_src->dec_streams[stream_index];
            auto frame_time_base = input_src->ifmt_ctx->streams[stream_index]->time_base;
            auto adjusted_pts = this->calc_adjusted_pts_time(frame, dec_stream);

            adjusted_pts -=
                av_rescale_q(dec_stream->input_start_pts, AV_TIME_BASE_Q, frame_time_base);

            m_frame_rpts = pts_to_rational(adjusted_pts, frame_time_base);

            m_frame_pts = (m_input_src->act_dur * m_input_src->loop_cnt) +
                          rpts_to_pts(m_frame_rpts, m_input_src->layer_prof.from,
                                      m_input_src->layer_prof.start);
        }

        bool PTSSet::is_valid(void) const {
            if (m_frame_pts < Rational(0, 1)) {
                AKLOG_INFO("FramePTS::is_valid(): Negative pts found {}", m_frame_pts.to_decimal());
                return false;
            }

            // [XXX] skip when the frame_pts is lower thant the current decode pts
            // this occurs when seeking
            if (m_frame_pts < m_input_src->dec_streams[m_stream_index].cur_decode_pts) {
                AKLOG_INFO(
                    "FramePTS::is_valid(): Frame pts is lower than the current dts, current: {}, frame: {}",
                    m_input_src->dec_streams[m_stream_index].cur_decode_pts.to_decimal(),
                    m_frame_pts.to_decimal());
                return false;
            }

            return true;
        }

        bool PTSSet::within_range(void) const {
            return m_input_src->layer_prof.from <= m_frame_pts &&
                   m_frame_pts <= m_input_src->layer_prof.to;
        }

        int64_t PTSSet::calc_adjusted_pts_time(const AVFrame* frame,
                                               const DecodeStream* dec_stream) const {
            // double frame_delay = 0;
            // if (dec_stream->first_pts != frame->pts) {
            //     frame_delay = av_q2d(dec_stream->dec_ctx->time_base);
            //     frame_delay += frame->repeat_pict * (frame_delay * 0.5);
            // }

            // [TODO] there might be bugs with this calculation, especially for the part with
            // first_pts_time when the value of start_time is positive, this calculation gives an
            // erroneous result if (dec_stream->first_pts_time < 0) {
            //     dec_stream->first_pts_time = dec_stream->input_start_time + frame_delay;
            //      }

            // [TODO] maybe we should check that the timebase of both values is same

            return frame->pts;
        }

    }
}
