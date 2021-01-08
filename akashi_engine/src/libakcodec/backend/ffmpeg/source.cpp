#include "./source.h"

#include "./input.h"
#include "./decode.h"
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
                                 const core::Rational& decode_start) {
            m_done_init = true;

            if (read_inputsrc(m_input_src, layer_profile.src.c_str()) < 0) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to parse input from argument");
                return false;
            }

            m_input_src->from = to_rational(layer_profile.from);
            m_input_src->to = to_rational(layer_profile.to);
            m_input_src->start = to_rational(layer_profile.start);
            m_input_src->uuid = layer_profile.uuid.c_str();

            for (auto&& dec_stream : m_input_src->dec_streams) {
                dec_stream.cur_decode_pts = std::max(m_input_src->from, decode_start);
            }

            auto seek_pts = m_input_src->start + (decode_start - m_input_src->from);
            // auto seek_pts = m_input_src->start + m_input_src->cur_decode_pts;
            if (seek_pts > Rational(0, 1)) {
                if (!this->seek(seek_pts)) {
                    return false;
                }
            }

            m_pkt = av_packet_alloc();
            if (!m_pkt) {
                AKLOG_ERRORN("FFLayerSource::init(): Failed to alloc packet");
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
            decode_result.layer_uuid = m_input_src->uuid;

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

                // bool within_range = m_input_src->from <= m_input_src->cur_decode_pts &&
                //                     m_input_src->cur_decode_pts <= m_input_src->to;
                // if (!within_range) {
                //     m_input_src->decode_ended = true;
                //     decode_result.result = DecodeResultCode::DECODE_LAYER_ENDED;
                //     goto exit;
                // }

                if (this->is_streams_end()) {
                    m_input_src->decode_ended = true;
                    decode_result.result = DecodeResultCode::DECODE_LAYER_ENDED;
                    goto exit;
                }

                // get a pkt
                ret_tmp = av_read_frame(m_input_src->ifmt_ctx, m_pkt);
                if (ret_tmp == AVERROR_EOF) {
                    m_input_src->decode_ended = true;
                    m_input_src->eof = 1;
                    AKLOG_DEBUGN("FFLayerSource::transcode_step(): eof reached");
                    decode_result.result = DecodeResultCode::DECODE_LAYER_EOF;
                    goto exit;
                } else if (ret_tmp < 0) {
                    AKLOG_ERROR("FFLayerSource::transcode_step(): av_read_frame failed, ret={}",
                                av_err2str(ret_tmp));
                    decode_result.result = DecodeResultCode::ERROR;
                    goto exit;
                }

                DecodeStream* dec_stream = &m_input_src->dec_streams[m_pkt->stream_index];

                // get a frame
                ret_tmp = decode_packet(m_pkt, m_frame, dec_stream->dec_ctx);
                if (ret_tmp == AVERROR(EAGAIN)) {
                    decode_result.result = DecodeResultCode::DECODE_AGAIN;
                    goto exit;
                } else if (ret_tmp < 0) {
                    AKLOG_ERROR("FFLayerSource::transcode_step(): decode_packet failed, ret={}",
                                av_err2str(ret_tmp));
                    decode_result.result = DecodeResultCode::ERROR;
                    goto exit;
                }

                // necessary for the pts calculation, but is it really necessary?
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

                if (!pts_set.within_range()) {
                    m_input_src->dec_streams[m_pkt->stream_index].decode_ended = true;
                    decode_result.result = DecodeResultCode::DECODE_STREAM_ENDED;
                    goto exit;
                }

                FFmpegBufferData::InputData ffbuf_input;

                ffbuf_input.frame = m_frame;
                ffbuf_input.start_frame = false; // [TODO] remove this
                ffbuf_input.pts = pts_set.frame_pts();
                ffbuf_input.rpts = pts_set.frame_rpts();
                ffbuf_input.out_audio_spec = out_audio_spec;
                ffbuf_input.uuid = m_input_src->uuid;
                ffbuf_input.media_type = to_res_buf_type(dec_stream->dec_ctx->codec_type);

                std::unique_ptr<FFmpegBufferData> buf_data(
                    new FFmpegBufferData(ffbuf_input, dec_stream));

                decode_result.buffer = std::move(buf_data);
                decode_result.layer_uuid = m_input_src->uuid;

                // update the state
                m_input_src->dec_streams[m_pkt->stream_index].cur_decode_pts = pts_set.frame_pts();
                decode_result.result = DecodeResultCode::OK;
            }

        exit:
            av_frame_unref(m_frame);
            av_packet_unref(m_pkt);
            return decode_result;
        }

        bool FFLayerSource::seek(const core::Rational& seek_pts) {
            for (size_t stream_idx = 0; stream_idx < m_input_src->ifmt_ctx->nb_streams;
                 stream_idx++) {
                auto media_type = m_input_src->dec_streams[stream_idx].media_type;
                if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO ||
                    media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
                    auto stream_time_base = m_input_src->dec_streams[stream_idx].dec_ctx->time_base;

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
            return !(m_input_src->eof || m_input_src->decode_ended);
        }

        bool FFLayerSource::is_streams_end(void) const {
            for (const auto& dec_stream : m_input_src->dec_streams) {
                if (!dec_stream.decode_ended) {
                    return false;
                }
            }
            return true;
        }

    }
}
