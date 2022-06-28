#include "./source.h"

#include "./input.h"
#include "./pts.h"
#include "./error.h"
#include "./buffer.h"
#include "./utils.h"
#include "../../source.h"
#include "../../decode_item.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}

#include <algorithm>

using namespace akashi::core;

namespace akashi {
    namespace codec {

        FFLayerSource::~FFLayerSource() { this->finalize(); }

        bool FFLayerSource::init(const core::LayerProfile& layer_profile,
                                 const core::Rational& decode_start,
                                 const core::VideoDecodeMethod& preferred_decode_method,
                                 const size_t video_max_queue_count) {
            m_done_init = true;

            if (read_inputsrc(m_input_src, layer_profile, preferred_decode_method,
                              video_max_queue_count) < 0) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to parse input from argument");
                return false;
            }

            auto r_dts = decode_start - m_input_src->layer_prof.from;
            if (r_dts < core::Rational(0, 1)) {
                m_input_src->loop_cnt = 0;
            } else {
                m_input_src->loop_cnt = (r_dts / m_input_src->act_dur).to_decimal();
            }

            for (auto&& dec_stream : m_input_src->dec_streams) {
                // [TODO] there might be accuracy issues here
                dec_stream.cur_decode_pts = std::max(m_input_src->layer_prof.from, decode_start);
            }

            auto act_from = m_input_src->layer_prof.from +
                            (core::Rational(m_input_src->loop_cnt, 1) * m_input_src->act_dur);

            auto seek_rpts = m_input_src->layer_prof.start + (decode_start - act_from);

            if (seek_rpts > Rational(0, 1)) {
                if (!this->seek(seek_rpts)) {
                    return false;
                }
            }

            m_pkt = av_packet_alloc();
            if (!m_pkt) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc packet");
                return false;
            }

            m_proxy_frame = av_frame_alloc();
            if (!m_proxy_frame) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc proxy frame");
                return false;
            }

            m_frame = av_frame_alloc();
            if (!m_frame) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc frame");
                return false;
            }

            return true;
        }

        DecodeResult FFLayerSource::decode(const DecodeArg& decode_arg) {
            DecodeResult decode_result;
            decode_result.layer_uuid = m_input_src->layer_prof.uuid;

            // [TODO] need refactoring
            auto out_audio_spec = decode_arg.out_audio_spec;
            {
                int ret_tmp = 0;

                if (m_input_src->eof) {
                    decode_result.result = DecodeResultCode::DECODE_LAYER_EOF;
                    goto exit;
                }
                if (m_input_src->decode_ended) {
                    decode_result.result = DecodeResultCode::DECODE_LAYER_ENDED;
                    goto exit;
                }

                if (this->is_streams_end()) {
                    m_input_src->decode_ended = true;
                    decode_result.result = DecodeResultCode::DECODE_LAYER_ENDED;
                    goto exit;
                }

                // get a pkt
                ret_tmp = av_read_frame(m_input_src->ifmt_ctx, m_pkt);
                if (ret_tmp == AVERROR_EOF) {
                    if (this->seek(m_input_src->layer_prof.start)) {
                        m_input_src->loop_cnt += 1;
                        decode_result.result = DecodeResultCode::DECODE_AGAIN;
                    } else {
                        m_input_src->decode_ended = true;
                        m_input_src->eof = 1;
                        AKLOG_DEBUGN("FFLayerSource::transcode_step(): eof reached");
                        decode_result.result = DecodeResultCode::DECODE_LAYER_EOF;
                    }
                    goto exit;
                } else if (ret_tmp < 0) {
                    AKLOG_ERROR("FFLayerSource::transcode_step(): av_read_frame failed, ret={}",
                                av_err2str(ret_tmp));
                    decode_result.result = DecodeResultCode::ERROR;
                    goto exit;
                }

                DecodeStream* dec_stream = &m_input_src->dec_streams[m_pkt->stream_index];
                if (!dec_stream->is_active) {
                    decode_result.result = DecodeResultCode::DECODE_SKIPPED;
                    goto exit;
                }

                // get a frame
                ret_tmp = this->decode_packet(m_pkt, m_proxy_frame, dec_stream->dec_ctx);
                if (ret_tmp == AVERROR(EAGAIN)) {
                    decode_result.result = DecodeResultCode::DECODE_AGAIN;
                    goto exit;
                } else if (ret_tmp < 0) {
                    AKLOG_ERROR("FFLayerSource::transcode_step(): decode_packet failed, ret={}",
                                av_err2str(ret_tmp));
                    decode_result.result = DecodeResultCode::ERROR;
                    goto exit;
                }

                if (m_proxy_frame->hw_frames_ctx &&
                    m_input_src->decode_method == VideoDecodeMethod::VAAPI_COPY) {
                    if (auto ret = av_hwframe_transfer_data(m_frame, m_proxy_frame, 0); ret < 0) {
                        AKLOG_ERROR("av_hwframe_transfer_data() failed, ret={}", av_err2str(ret));
                        goto exit;
                    } else {
                        // [XXX] be careful that some fields of m_frame are nuked!
                        // no need to re-set width, height, format, linesize to m_frame!
                        m_frame->pts = m_proxy_frame->pts;
                    }
                } else {
                    av_frame_ref(m_frame, m_proxy_frame);
                }

                if (!dec_stream->is_checked_first_pts) {
                    dec_stream->first_pts = m_frame->pts;
                    dec_stream->is_checked_first_pts = true;
                }

                // pts calculation
                PTSSet pts_set(m_input_src, m_frame, m_pkt->stream_index);

                if (!pts_set.is_valid()) {
                    AKLOG_INFON("FFLayerSource::transcode_step(): Invalid pts found. Skipped");
                    decode_result.result = DecodeResultCode::DECODE_SKIPPED;
                    goto exit;
                }

                if (pts_set.frame_rpts() > m_input_src->layer_prof.end) {
                    if (this->seek(m_input_src->layer_prof.start)) {
                        m_input_src->loop_cnt += 1;
                        decode_result.result = DecodeResultCode::DECODE_AGAIN;
                    } else {
                        m_input_src->decode_ended = true;
                        decode_result.result = DecodeResultCode::ERROR;
                    }
                }

                if (!pts_set.within_range()) {
                    m_input_src->dec_streams[m_pkt->stream_index].decode_ended = true;
                    decode_result.result = DecodeResultCode::DECODE_STREAM_ENDED;
                    goto exit;
                }

                FFmpegBufferData::InputData ffbuf_input;

                if (m_input_src->decode_method == VideoDecodeMethod::VAAPI) {
                    AVFrame* new_frame = av_frame_alloc();
                    av_frame_ref(new_frame, m_frame);
                    ffbuf_input.frame = new_frame;
                } else {
                    ffbuf_input.frame =
                        m_frame; // [XXX] after this, `m_frame` should not be accessed
                }
                ffbuf_input.pts = pts_set.frame_pts();
                ffbuf_input.rpts = pts_set.frame_rpts();
                ffbuf_input.out_audio_spec = out_audio_spec;
                ffbuf_input.media_type = to_res_buf_type(dec_stream->dec_ctx->codec_type);
                ffbuf_input.decode_method = m_input_src->decode_method;

                if (m_input_src->decode_method == VideoDecodeMethod::VAAPI) {
                    auto raw_hw_device_ctx = (AVHWDeviceContext*)m_input_src->hw_device_ctx->data;
                    ffbuf_input.va_display =
                        static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display;
                }

                std::unique_ptr<FFmpegBufferData> buf_data(
                    new FFmpegBufferData(ffbuf_input, dec_stream));

                decode_result.buffer = std::move(buf_data);
                decode_result.layer_uuid = m_input_src->layer_prof.uuid;

                m_input_src->dec_streams[m_pkt->stream_index].cur_decode_pts = pts_set.frame_pts();
                decode_result.result = DecodeResultCode::OK;
            }

        exit:
            av_frame_unref(m_frame);
            av_frame_unref(m_proxy_frame);
            av_packet_unref(m_pkt);
            return decode_result;
        }

        bool FFLayerSource::seek(const core::Rational& seek_pts) {
            for (size_t stream_idx = 0; stream_idx < m_input_src->ifmt_ctx->nb_streams;
                 stream_idx++) {
                if (!m_input_src->dec_streams[stream_idx].is_active) {
                    continue;
                }
                auto media_type = m_input_src->dec_streams[stream_idx].media_type;

                if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO ||
                    media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
                    auto stream_time_base = m_input_src->ifmt_ctx->streams[stream_idx]->time_base;

                    auto dst_pts = av_rescale_q(seek_pts.num(),
                                                (AVRational){1, static_cast<int>(seek_pts.den())},
                                                stream_time_base);

                    // long dst_pts = 0;
                    // if (stream_time_base.num == 1) {
                    //     dst_pts = av_rescale(seek_value.num(), seek_value.den(),
                    //                          stream_time_base.den);
                    // } else {
                    //     dst_pts =
                    //         av_rescale_q(seek_value.num(),
                    //                      (AVRational){1, static_cast<int>(seek_value.den())},
                    //                      stream_time_base);
                    // }

                    if (av_seek_frame(m_input_src->ifmt_ctx, stream_idx, dst_pts,
                                      AVSEEK_FLAG_BACKWARD) < 0) {
                        AKLOG_ERRORN("FFLayerSource::seek(): Seek Failed");
                        return false;
                    } else {
                        avcodec_flush_buffers(m_input_src->dec_streams[stream_idx].dec_ctx);
                    }
                }
            }
            return true;
        }

        void FFLayerSource::finalize(void) {
            if (m_pkt) {
                av_packet_free(&m_pkt);
                m_pkt = nullptr;
            }
            if (m_proxy_frame) {
                av_frame_free(&m_proxy_frame);
                m_proxy_frame = nullptr;
            }
            if (m_frame) {
                av_frame_free(&m_frame);
                m_frame = nullptr;
            }

            if (m_input_src) {
                free_input_src(m_input_src);
            }
            m_input_src = nullptr;
        }

        bool FFLayerSource::can_decode(void) const {
            return m_input_src->eof != 1 && !m_input_src->decode_ended;
        }

        core::Rational FFLayerSource::dts(void) const {
            if (m_input_src->dec_streams.size() == 0) {
                AKLOG_ERRORN("FFLayerSource::dts(): No streams found");
                return core::Rational(0, 1);
            }
            core::Rational res_dts = core::Rational(INT32_MAX, 1);
            for (const auto& dec_stream : m_input_src->dec_streams) {
                res_dts = std::min(dec_stream.cur_decode_pts, res_dts);
            }
            return res_dts;
        }

        const core::LayerProfile& FFLayerSource::layer_profile() const {
            return m_input_src->layer_prof;
        }

        // return 0 if success, -11 if EAGAIN.
        // If error occurs, other negative value will be returned.
        int FFLayerSource::decode_packet(AVPacket* pkt, AVFrame* frame, AVCodecContext* dec_ctx) {
            int ret_code = 0;

            if ((ret_code = avcodec_send_packet(dec_ctx, pkt)) < 0) {
                AKLOG_ERROR("decode_packet(): avcodec_send_packet error. ret={}",
                            AVERROR(ret_code));
                goto exit;
            }

            if ((ret_code = avcodec_receive_frame(dec_ctx, frame)) < 0) {
                if (ret_code != AVERROR(EAGAIN)) {
                    AKLOG_ERROR("decode_packet(): avcodec_receive_frame error. ret={}",
                                AVERROR(ret_code));
                }
                goto exit;
            }

        exit:
            return ret_code;
        }

        bool FFLayerSource::is_streams_end(void) const {
            for (const auto& dec_stream : m_input_src->dec_streams) {
                if (!dec_stream.is_active) {
                    continue;
                }
                if (!dec_stream.decode_ended) {
                    return false;
                }
            }
            return true;
        }

    }
}
