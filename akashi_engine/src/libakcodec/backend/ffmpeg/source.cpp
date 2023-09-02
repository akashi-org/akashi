#include "./source.h"

#include "./hwaccel.h"
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
}

#include <algorithm>

using namespace akashi::core;

namespace akashi {
    namespace codec {

        FFLayerSource::~FFLayerSource() { this->finalize(); }

        bool FFLayerSource::init(const core::LayerProfile& layer_profile,
                                 const core::Rational& decode_start,
                                 const DecodeArg& init_decode_arg) {
            m_input_src.init_called = true;

            if (!this->init_inputsrc(layer_profile, decode_start, init_decode_arg)) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to parse input from argument");
                return false;
            }

            auto act_from = m_input_src.layer_prof.from +
                            (core::Rational(m_input_src.loop_cnt, 1) * m_input_src.act_dur);

            auto media_offset = m_input_src.layer_prof.layer_local_offset;

            auto seek_rpts =
                (media_offset + m_input_src.layer_prof.start) + (decode_start - act_from);

            if (seek_rpts > Rational(0, 1)) {
                if (!this->seek(seek_rpts)) {
                    return false;
                }
            }

            return true;
        }

        DecodeResult FFLayerSource::decode(const DecodeArg& decode_arg) {
            DecodeResult decode_result;
            decode_result.layer_uuid = m_input_src.layer_prof.uuid;

            {
                // sanity checks
                if (m_input_src.eof || m_input_src.decode_ended) {
                    decode_result.result = m_input_src.eof ? DecodeResultCode::DECODE_LAYER_EOF
                                                           : DecodeResultCode::DECODE_LAYER_ENDED;
                    goto exit;
                }
                if (this->all_streams_ended()) {
                    m_input_src.decode_ended = true;
                    decode_result.result = DecodeResultCode::DECODE_LAYER_ENDED;
                    goto exit;
                }

                // demux & decode
                if (!this->demux_priv(&decode_result) || !this->decode_priv(&decode_result)) {
                    goto exit;
                }

                // validate & polulate
                PTSSet pts_set(&m_input_src, m_input_src.frame, m_input_src.pkt->stream_index);
                if (!this->validate_pts(&decode_result, pts_set)) {
                    goto exit;
                }
                this->populate_buffer(&decode_result, decode_arg, pts_set);

                // update decode state
                m_input_src.dec_streams[m_input_src.pkt->stream_index].cur_decode_pts =
                    pts_set.frame_pts();
            }

        exit:
            if (m_input_src.frame) {
                av_frame_unref(m_input_src.frame);
            }
            if (m_input_src.proxy_frame) {
                av_frame_unref(m_input_src.proxy_frame);
            }
            if (m_input_src.pkt) {
                av_packet_unref(m_input_src.pkt);
            }
            return decode_result;
        }

        bool FFLayerSource::seek(const core::Rational& seek_pts) {
            for (size_t stream_idx = 0; stream_idx < m_input_src.ifmt_ctx->nb_streams;
                 stream_idx++) {
                if (!m_input_src.dec_streams[stream_idx].is_active) {
                    continue;
                }
                auto media_type = m_input_src.dec_streams[stream_idx].media_type;

                if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO ||
                    media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
                    auto stream_time_base = m_input_src.ifmt_ctx->streams[stream_idx]->time_base;

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

                    if (av_seek_frame(m_input_src.ifmt_ctx, stream_idx, dst_pts,
                                      AVSEEK_FLAG_BACKWARD) < 0) {
                        AKLOG_ERRORN("FFLayerSource::seek(): Seek Failed");
                        return false;
                    } else {
                        avcodec_flush_buffers(m_input_src.dec_streams[stream_idx].dec_ctx);
                    }
                }
            }
            return true;
        }

        void FFLayerSource::finalize(void) {
            if (m_input_src.pkt) {
                av_packet_free(&m_input_src.pkt);
                m_input_src.pkt = nullptr;
            }
            if (m_input_src.proxy_frame) {
                av_frame_free(&m_input_src.proxy_frame);
                m_input_src.proxy_frame = nullptr;
            }
            if (m_input_src.frame) {
                av_frame_free(&m_input_src.frame);
                m_input_src.frame = nullptr;
            }

            if (m_input_src.ifmt_ctx != nullptr) {
                for (auto&& dec_stream : m_input_src.dec_streams) {
                    if (dec_stream.dec_ctx != nullptr) {
                        avcodec_free_context(&dec_stream.dec_ctx);
                        dec_stream.dec_ctx = nullptr;
                    }
                    if (dec_stream.swr_ctx != nullptr) {
                        // [TODO] need to flush the internal buffer before freeing?
                        swr_free(&dec_stream.swr_ctx);
                        dec_stream.swr_ctx = nullptr;
                    }
                }
                avformat_close_input(&m_input_src.ifmt_ctx);
                avformat_free_context(m_input_src.ifmt_ctx);
            }

            if (m_input_src.hw_device_ctx != nullptr) {
                av_buffer_unref(&m_input_src.hw_device_ctx);
                m_input_src.hw_device_ctx = nullptr;
            }
        }

        /* --- getter methods --- */

        bool FFLayerSource::can_decode(void) const {
            return m_input_src.eof != 1 && !m_input_src.decode_ended;
        }

        bool FFLayerSource::done_init(void) const { return m_input_src.init_called; }

        core::Rational FFLayerSource::dts(void) const {
            if (m_input_src.dec_streams.size() == 0) {
                AKLOG_ERRORN("FFLayerSource::dts(): No streams found");
                return core::Rational(0, 1);
            }
            core::Rational res_dts = core::Rational(INT32_MAX, 1);
            for (const auto& dec_stream : m_input_src.dec_streams) {
                res_dts = std::min(dec_stream.cur_decode_pts, res_dts);
            }
            return res_dts;
        }

        const core::LayerProfile& FFLayerSource::layer_profile() const {
            return m_input_src.layer_prof;
        }

        /* --- private methods --- */

        bool FFLayerSource::init_inputsrc(const core::LayerProfile& layer_profile,
                                          const core::Rational& decode_start,
                                          const DecodeArg& init_decode_arg) {
            m_input_src.layer_prof = layer_profile;
            m_input_src.act_dur = layer_profile.end - layer_profile.start;
            m_input_src.preferred_decode_method = init_decode_arg.preferred_decode_method;
            m_input_src.video_max_queue_count = init_decode_arg.video_max_queue_count;

            if (m_input_src.preferred_decode_method == core::VideoDecodeMethod::SW) {
                m_input_src.decode_method = init_decode_arg.preferred_decode_method;
            }

            auto r_dts = decode_start - m_input_src.layer_prof.from;
            if (r_dts < core::Rational(0, 1)) {
                m_input_src.loop_cnt = 0;
            } else {
                m_input_src.loop_cnt = (r_dts / m_input_src.act_dur).to_decimal();
                if (m_input_src.layer_prof.layer_local_offset > 0 and
                    r_dts > (m_input_src.act_dur - m_input_src.layer_prof.layer_local_offset)) {
                    m_input_src.loop_cnt += 1;
                }
            }

            // input_src[i].io_ctx = new IOContext(input_paths[i]);
            if (this->read_avformat() < 0) {
                return false;
            }
            if ((this->read_avstream(init_decode_arg) < 0)) {
                return false;
            }

            for (auto&& dec_stream : m_input_src.dec_streams) {
                // [TODO] there might be accuracy issues here
                dec_stream.cur_decode_pts = std::max(m_input_src.layer_prof.from, decode_start);
            }

            m_input_src.pkt = av_packet_alloc();
            if (!m_input_src.pkt) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc packet");
                return false;
            }

            m_input_src.proxy_frame = av_frame_alloc();
            if (!m_input_src.proxy_frame) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc proxy frame");
                return false;
            }

            m_input_src.frame = av_frame_alloc();
            if (!m_input_src.frame) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc frame");
                return false;
            }

            return true;
        }

        int FFLayerSource::read_avformat() {
            int ret = 0;

            ret = avformat_open_input(&(m_input_src.ifmt_ctx), m_input_src.layer_prof.src.c_str(),
                                      nullptr, nullptr);

            if (ret < 0) {
                AKLOG_ERROR("avformat_open_input() failed, code={}({})", AVERROR(ret),
                            av_err2str(ret));
                goto end;
            }

            ret = avformat_find_stream_info(m_input_src.ifmt_ctx, nullptr);
            if (ret < 0) {
                AKLOG_ERROR("avformat_find_stream_info() failed, code={}({})", AVERROR(ret),
                            av_err2str(ret));
                goto end;
            }
            if (m_input_src.ifmt_ctx == nullptr) {
                ret = -1;
                AKLOG_ERRORN("failed to get avformat context");
                goto end;
            }

        end:
            return ret;
        }

        int FFLayerSource::read_avstream(const DecodeArg& init_decode_arg) {
            AVFormatContext* format_ctx = m_input_src.ifmt_ctx;
            m_input_src.dec_streams.reserve(format_ctx->nb_streams);
            m_input_src.dec_streams.resize(format_ctx->nb_streams);

            for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
                AVMediaType media_type = format_ctx->streams[i]->codecpar->codec_type;
                m_input_src.dec_streams[i].media_type = media_type;
                if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO ||
                    media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
                    // skip a video stream if audio only
                    if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
                        if (m_input_src.layer_prof.type & core::MediaFlagAudio) {
                            m_input_src.dec_streams[i].is_active = false;
                            continue;
                        }
                    }

                    AVCodecID codec_id = format_ctx->streams[i]->codecpar->codec_id;
                    auto av_codec = avcodec_find_decoder(codec_id);
                    if (av_codec == nullptr) {
                        AKLOG_ERROR("avcodec_find_decoder codec not found. codec_id={}", codec_id);
                        return -1;
                    }

                    if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO &&
                        static_cast<int>(m_input_src.preferred_decode_method) > 0) {
                        if (validate_hwconfig(&m_input_src.decode_method, av_codec,
                                              m_input_src.preferred_decode_method)) {
                            // [TODO] error handling?
                            codec::create_hwdevice_ctx(m_input_src.hw_device_ctx,
                                                       m_input_src.decode_method,
                                                       init_decode_arg.vaapi_device);
                        }
                    }

                    m_input_src.dec_streams[i].is_active = true;
                    m_input_src.dec_streams[i].dec_ctx = avcodec_alloc_context3(av_codec);
                    m_input_src.dec_streams[i].swr_ctx = nullptr;
                    m_input_src.dec_streams[i].swr_ctx_init_done = false;
                    m_input_src.dec_streams[i].is_checked_first_rpts = false;
                    m_input_src.dec_streams[i].input_start_pts =
                        format_ctx->start_time == AV_NOPTS_VALUE ? 0 : format_ctx->start_time;
                    m_input_src.dec_streams[i].cur_decode_pts = akashi::core::Rational(0, 1);

                    AVCodecContext* codec_ctx = m_input_src.dec_streams[i].dec_ctx;

                    if (codec_ctx == nullptr) {
                        AKLOG_ERRORN("avcodec_alloc_context3() error");
                        return -1;
                    }

                    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[i]->codecpar) <
                        0) {
                        AKLOG_ERRORN("avcodec_parameters_to_context() error");
                        return -1;
                    }

                    if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO &&
                        m_input_src.hw_device_ctx) {
                        codec_ctx->hw_device_ctx = av_buffer_ref(m_input_src.hw_device_ctx);
                        if (!codec_ctx->hw_device_ctx) {
                            AKLOG_ERRORN("A hardware device reference create failed");
                            return -1;
                        }
                        codec_ctx->get_format = codec::configure_hwformat;
                        // [TODO] lifetime?
                        codec_ctx->opaque = &m_input_src;
                    }

                    int ret = 0;
                    ret = avcodec_open2(codec_ctx, av_codec, nullptr);
                    if (ret < 0) {
                        AKLOG_ERROR("avcodec_open2() failed, code={}", AVERROR(ret));
                        return -1;
                    }
                }
            }
            return 1;
        }

        bool FFLayerSource::demux_priv(DecodeResult* decode_result) {
            int ret = av_read_frame(m_input_src.ifmt_ctx, m_input_src.pkt);
            if (ret == AVERROR_EOF) {
                if (this->seek(m_input_src.layer_prof.start)) {
                    m_input_src.loop_cnt += 1;
                    decode_result->result = DecodeResultCode::DECODE_AGAIN;
                } else {
                    m_input_src.decode_ended = true;
                    m_input_src.eof = 1;
                    AKLOG_DEBUGN("eof reached");
                    decode_result->result = DecodeResultCode::DECODE_LAYER_EOF;
                }
                return false;
            } else if (ret < 0) {
                AKLOG_ERROR("av_read_frame() failed, ret={}", av_err2str(ret));
                decode_result->result = DecodeResultCode::ERROR;
                return false;
            }

            auto dec_stream = &m_input_src.dec_streams[m_input_src.pkt->stream_index];
            if (!dec_stream->is_active) {
                decode_result->result = DecodeResultCode::DECODE_SKIPPED;
                return false;
            }

            return true;
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

        bool FFLayerSource::decode_priv(DecodeResult* decode_result) {
            auto dec_stream = &m_input_src.dec_streams[m_input_src.pkt->stream_index];

            int ret =
                this->decode_packet(m_input_src.pkt, m_input_src.proxy_frame, dec_stream->dec_ctx);
            if (ret == AVERROR(EAGAIN)) {
                decode_result->result = DecodeResultCode::DECODE_AGAIN;
                return false;
            } else if (ret < 0) {
                AKLOG_ERROR("decode_packet failed, ret={}", av_err2str(ret));
                decode_result->result = DecodeResultCode::ERROR;
                return false;
            }

            if (m_input_src.proxy_frame->hw_frames_ctx &&
                m_input_src.decode_method == VideoDecodeMethod::VAAPI_COPY) {
                if (auto ret =
                        av_hwframe_transfer_data(m_input_src.frame, m_input_src.proxy_frame, 0);
                    ret < 0) {
                    AKLOG_ERROR("av_hwframe_transfer_data() failed, ret={}", av_err2str(ret));
                    return false;
                } else {
                    // [XXX] be careful that some fields of m_input_src.frame are nuked!
                    // no need to re-set width, height, format, linesize to m_input_src.frame!
                    m_input_src.frame->pts = m_input_src.proxy_frame->pts;
                }
            } else {
                av_frame_ref(m_input_src.frame, m_input_src.proxy_frame);
            }

            return true;
        }

        bool FFLayerSource::validate_pts(DecodeResult* decode_result, const PTSSet& pts_set) {
            if (!pts_set.is_valid()) {
                AKLOG_INFON("Invalid pts found. Skipped");
                decode_result->result = DecodeResultCode::DECODE_SKIPPED;
                return false;
            }

            if (pts_set.frame_rpts() > m_input_src.layer_prof.end) {
                if (this->seek(m_input_src.layer_prof.start)) {
                    m_input_src.loop_cnt += 1;
                    decode_result->result = DecodeResultCode::DECODE_AGAIN;
                } else {
                    m_input_src.decode_ended = true;
                    decode_result->result = DecodeResultCode::ERROR;
                }
                return false;
            }

            auto dec_stream = &m_input_src.dec_streams[m_input_src.pkt->stream_index];

            if (!pts_set.within_range()) {
                dec_stream->decode_ended = true;
                decode_result->result = DecodeResultCode::DECODE_STREAM_ENDED;
                return false;
            }

            if (!dec_stream->is_checked_first_rpts) {
                dec_stream->first_rpts = pts_set.frame_rpts();
                dec_stream->is_checked_first_rpts = true;
            }

            return true;
        }

        void FFLayerSource::populate_buffer(DecodeResult* decode_result,
                                            const DecodeArg& decode_arg, const PTSSet& pts_set) {
            FFFrameData ffbuf_input;

            auto dec_stream = &m_input_src.dec_streams[m_input_src.pkt->stream_index];

            if (m_input_src.decode_method == VideoDecodeMethod::VAAPI) {
                AVFrame* new_frame = av_frame_alloc();
                av_frame_ref(new_frame, m_input_src.frame);
                ffbuf_input.frame = new_frame;
            } else {
                // [XXX] after this, `m_input_src.frame` should not be accessed
                ffbuf_input.frame = m_input_src.frame;
            }
            ffbuf_input.pts = pts_set.frame_pts();
            ffbuf_input.rpts = pts_set.frame_rpts();
            ffbuf_input.out_audio_spec = decode_arg.out_audio_spec;
            ffbuf_input.media_type = to_res_buf_type(dec_stream->dec_ctx->codec_type);
            ffbuf_input.decode_method = m_input_src.decode_method;
            ffbuf_input.layer_prof = m_input_src.layer_prof;

            if (m_input_src.decode_method == VideoDecodeMethod::VAAPI) {
                codec::init_hwframe(ffbuf_input, m_input_src);
            }

            std::unique_ptr<FFmpegBufferData> buf_data(
                new FFmpegBufferData(ffbuf_input, dec_stream));

            decode_result->buffer = std::move(buf_data);
            decode_result->layer_uuid = m_input_src.layer_prof.uuid;
            decode_result->result = DecodeResultCode::OK;
        }

        bool FFLayerSource::all_streams_ended(void) const {
            for (const auto& dec_stream : m_input_src.dec_streams) {
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
