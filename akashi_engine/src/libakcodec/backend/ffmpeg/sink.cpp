#include "./sink.h"

#include "./error.h"
#include "./utils.h"
#include "../../encode_item.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

using namespace akashi::core;

namespace akashi {
    namespace codec {

        FFFrameSink::FFFrameSink(core::borrowed_ptr<state::AKState> state)
            : FrameSink(state), m_state(state){};

        FFFrameSink::~FFFrameSink() {
            // video stream
            if (m_video_stream.enc_ctx) {
                if (!m_encoder_flushed) {
                    this->flush_encoder(buffer::AVBufferType::VIDEO);
                }
                if (m_video_stream.sws_ctx) {
                    sws_freeContext(m_video_stream.sws_ctx);
                    m_video_stream.sws_ctx = nullptr;
                }
                avcodec_free_context(&m_video_stream.enc_ctx);
                m_video_stream.enc_ctx = nullptr;
                m_video_stream.enc_stream = nullptr;
            }
            // audio stream
            if (m_audio_stream.enc_ctx) {
                if (!m_encoder_flushed) {
                    this->flush_encoder(buffer::AVBufferType::AUDIO);
                }
                avcodec_free_context(&m_audio_stream.enc_ctx);
                m_audio_stream.enc_ctx = nullptr;
                m_audio_stream.enc_stream = nullptr;
            }
            m_encoder_flushed = true;
            // ofmt
            if (m_ofmt_ctx) {
                if (m_ofmt_ctx->pb) {
                    if (auto err = avio_closep(&m_ofmt_ctx->pb); err < 0) {
                        AKLOG_WARN("avio_close failed, ret={}", av_err2str(err));
                    }
                }
                avformat_close_input(&m_ofmt_ctx);
                m_ofmt_ctx = nullptr;
            }
        }

        bool FFFrameSink::open(void) {
            auto out_fname = m_state->m_encode_conf.out_fname;
            if (auto err = avformat_alloc_output_context2(&m_ofmt_ctx, nullptr, nullptr,
                                                          out_fname.c_str());
                err < 0) {
                AKLOG_ERROR("avformat_alloc_output_context2() failed, ret={}", av_err2str(err));
                return false;
            }

            // init streams
            if (m_state->m_encode_conf.video_codec != "" && !this->init_video_stream()) {
                return false;
            }
            if (m_state->m_encode_conf.audio_codec != "" && !this->init_audio_stream()) {
                return false;
            }

            // init io
            if (auto err = avio_open(&m_ofmt_ctx->pb, out_fname.c_str(), AVIO_FLAG_WRITE);
                err < 0) {
                AKLOG_ERROR("avio_open() failed, ret={}", av_err2str(err));
                return false;
            }

            if (auto err = avformat_write_header(m_ofmt_ctx, nullptr); err < 0) {
                AKLOG_ERROR("avformat_write_header() failed, ret={}", av_err2str(err));
                return false;
            }
            return true;
        }

        bool FFFrameSink::close(void) {
            if (m_video_stream.enc_ctx && !m_encoder_flushed) {
                this->flush_encoder(buffer::AVBufferType::VIDEO);
            }
            if (m_audio_stream.enc_ctx && !m_encoder_flushed) {
                this->flush_encoder(buffer::AVBufferType::AUDIO);
            }
            m_encoder_flushed = true;
            if (m_ofmt_ctx) {
                av_write_trailer(m_ofmt_ctx);
            }
            return true;
        }

        EncodeResultCode FFFrameSink::send(const EncodeArg& encode_arg) {
            // init avframe
            AVFrame* frame = nullptr;
            AVCodecContext* enc_ctx = nullptr;
            AVStream* enc_stream = nullptr;

            switch (encode_arg.type) {
                case buffer::AVBufferType::VIDEO: {
                    if (!this->init_video_frame(&frame, encode_arg)) {
                        av_frame_free(&frame);
                        return EncodeResultCode::ERROR;
                    }
                    if (!this->populate_video_frame(frame, encode_arg)) {
                        av_frame_free(&frame);
                        return EncodeResultCode::ERROR;
                    }
                    enc_ctx = m_video_stream.enc_ctx;
                    enc_stream = m_video_stream.enc_stream;
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    if (!this->init_audio_frame(&frame, encode_arg)) {
                        av_frame_free(&frame);
                        return EncodeResultCode::ERROR;
                    }
                    if (!this->populate_audio_frame(frame, encode_arg)) {
                        av_frame_free(&frame);
                        return EncodeResultCode::ERROR;
                    }
                    enc_ctx = m_audio_stream.enc_ctx;
                    enc_stream = m_audio_stream.enc_stream;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid type found, {}", encode_arg.type);
                    av_frame_free(&frame);
                    return EncodeResultCode::ERROR;
                }
            }

            // send_frame
            if (auto err = avcodec_send_frame(enc_ctx, frame); err < 0) {
                av_frame_free(&frame);
                if (err == AVERROR(EAGAIN)) {
                    return EncodeResultCode::SEND_EAGAIN;
                } else {
                    AKLOG_ERROR("avcodec_send_frame() failed, ret={}", av_err2str(err));
                    return EncodeResultCode::ERROR;
                }
            }

            if (frame) {
                av_frame_free(&frame);
            }

            return EncodeResultCode::OK;
        };

        EncodeWriteResult FFFrameSink::write(const EncodeWriteArg& write_arg) {
            AVPacket* pkt = av_packet_alloc();
            EncodeResultCode result = EncodeResultCode::NONE;
            AVCodecContext* enc_ctx = nullptr;
            AVStream* enc_stream = nullptr;

            switch (write_arg.type) {
                case buffer::AVBufferType::VIDEO: {
                    enc_ctx = m_video_stream.enc_ctx;
                    enc_stream = m_video_stream.enc_stream;
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    enc_ctx = m_audio_stream.enc_ctx;
                    enc_stream = m_audio_stream.enc_stream;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid type found, {}", write_arg.type);
                    result = EncodeResultCode::ERROR;
                    goto exit;
                }
            }

            // recv pkt and write it to file
            {
                auto err = avcodec_receive_packet(enc_ctx, pkt);
                if (err == AVERROR(EAGAIN)) {
                    result = EncodeResultCode::RECV_EAGAIN;
                    goto exit;
                } else if (err == AVERROR_EOF) {
                    result = EncodeResultCode::RECV_EOF;
                    goto exit;
                } else if (err < 0) {
                    AKLOG_ERROR("avcodec_receive_packet() failed, ret={}", av_err2str(err));
                    result = EncodeResultCode::ERROR;
                    goto exit;
                }

                pkt->stream_index = enc_stream->index;
                av_packet_rescale_ts(pkt, enc_ctx->time_base, enc_stream->time_base);

                auto pkt_pts = Rational(pkt->pts) * to_rational(enc_stream->time_base);

                if (auto err = av_interleaved_write_frame(m_ofmt_ctx, pkt); err < 0) {
                    AKLOG_ERROR("av_interleaved_write_frame() failed, ret={}", av_err2str(err));
                    // break or return?
                }

                AKLOG_WARN("Frame (type: {}, pts: {}) written", write_arg.type,
                           pkt_pts.to_decimal());

                // [TODO] really necessary?
                // see av_interleaved_write_frame()'s documentation
                // av_packet_unref(pkt);
            }
            result = EncodeResultCode::OK;

        exit:
            if (pkt) {
                av_packet_free(&pkt);
            }
            return {.result = result};
        }

        size_t FFFrameSink::nb_samples_per_frame(void) {
            if (m_audio_stream.enc_ctx) {
                return m_audio_stream.enc_ctx->frame_size;
            }
            // in case where AV_CODEC_CAP_VARIABLE_FRAME_SIZE is set
            else {
                if (m_video_stream.enc_ctx) {
                    Rational fps;
                    int sample_rate;
                    {
                        std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                        fps = m_state->m_prop.fps;
                        sample_rate = m_state->m_atomic_state.encode_audio_spec.load().sample_rate;
                    }
                    AKLOG_WARNN(
                        "Could not find the appropriate nb_samples per frame. Using calculated number for it");
                    return static_cast<size_t>(
                        ((Rational(1l) / fps) * Rational(sample_rate, 1)).to_decimal());
                } else {
                    AKLOG_WARNN(
                        "Could not find the appropriate nb_samples per frame. Using arbitrary number for it");
                    return 256;
                }
            }
        }

        core::AKAudioSampleFormat
        FFFrameSink::validate_audio_format(const core::AKAudioSampleFormat& sample_format) {
            // find codec
            auto codec = avcodec_find_encoder_by_name(m_state->m_encode_conf.audio_codec.c_str());
            if (!codec) {
                AKLOG_ERROR("avcodec_find_encoder_by_name() failed, codec_name: {}",
                            m_state->m_encode_conf.audio_codec.c_str());
                return core::AKAudioSampleFormat::NONE;
            }

            const enum AVSampleFormat* p = codec->sample_fmts;
            auto ff_sample_fmt = to_ff_sample_format(sample_format);
            auto ff_sample_fmt_alt = av_get_alt_sample_fmt(
                ff_sample_fmt, av_sample_fmt_is_planar(ff_sample_fmt) ? 0 : 1);
            bool success = false;
            bool alt_success = false;
            while (*p != AV_SAMPLE_FMT_NONE) {
                if (*p == ff_sample_fmt) {
                    success = true;
                    break;
                }
                if (*p == ff_sample_fmt_alt) {
                    alt_success = true;
                }
                p++;
            }
            if (!success) {
                if (alt_success) {
                    AKLOG_INFO("Using alt sample format `{}`",
                               av_get_sample_fmt_name(ff_sample_fmt_alt));
                    return from_ff_sample_format(ff_sample_fmt_alt);
                }
                AKLOG_ERROR("Not supported sample format `{}` found",
                            av_get_sample_fmt_name(ff_sample_fmt));
                return core::AKAudioSampleFormat::NONE;
            } else {
                return from_ff_sample_format(ff_sample_fmt);
            }
        }

        bool FFFrameSink::init_video_stream() {
            // find codec
            auto codec = avcodec_find_encoder_by_name(m_state->m_encode_conf.video_codec.c_str());
            if (!codec) {
                AKLOG_ERROR("avcodec_find_encoder_by_name() failed, codec_name: {}",
                            m_state->m_encode_conf.video_codec.c_str());
                return false;
            }

            // alloc codec ctx
            m_video_stream.enc_ctx = avcodec_alloc_context3(codec);
            if (!m_video_stream.enc_ctx) {
                AKLOG_ERRORN("avcodec_alloc_context3() failed");
                return false;
            }
            auto enc_ctx = m_video_stream.enc_ctx;

            // codec ctx settings
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                enc_ctx->width = m_state->m_prop.video_width;
                enc_ctx->height = m_state->m_prop.video_height;
                enc_ctx->time_base = to_av_rational(Rational(1l) / m_state->m_prop.fps);
                enc_ctx->framerate = to_av_rational(m_state->m_prop.fps);
                enc_ctx->colorspace = AVColorSpace::AVCOL_SPC_BT709;
                // enc_ctx->color_primaries = AVColorPrimaries::AVCOL_PRI_BT709;
                // enc_ctx->color_trc = AVColorTransferCharacteristic::AVCOL_TRC_BT709;
                enc_ctx->color_range = AVColorRange::AVCOL_RANGE_MPEG;
                // [TODO] add a field for AKState
                enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
                // [XXX] settings for interlacing?
            }

            if (m_ofmt_ctx->flags & AVFMT_GLOBALHEADER) {
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            // open encoder
            // codec_opts?
            if (auto err = avcodec_open2(enc_ctx, codec, nullptr); err < 0) {
                AKLOG_ERROR("avcodec_open2() failed, ret={}", av_err2str(err));
                return false;
            }

            // init stream
            m_video_stream.enc_stream = avformat_new_stream(m_ofmt_ctx, codec);
            if (!m_video_stream.enc_stream) {
                AKLOG_ERRORN("avformat_new_stream() failed");
                return false;
            }

            if (auto err =
                    avcodec_parameters_from_context(m_video_stream.enc_stream->codecpar, enc_ctx);
                err < 0) {
                AKLOG_ERROR("avcodec_parameters_from_context() failed, ret={}", av_err2str(err));
                return false;
            }

            // init sws ctx
            // clang-format off
            m_video_stream.sws_ctx = sws_getCachedContext(nullptr,
                // src
                enc_ctx->width, enc_ctx->height, AV_PIX_FMT_RGB24,
                // dst
                enc_ctx->width, enc_ctx->height, enc_ctx->pix_fmt,
                // flags
                SWS_BICUBIC | SWS_FULL_CHR_H_INP | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND, // SWS_LANCZOS | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
                // options
                nullptr, nullptr, nullptr
            );
            // clang-format on
            if (!m_video_stream.sws_ctx) {
                AKLOG_ERRORN("sws_getCashedContext() failed");
                return false;
            }

            // NB: This operation is really important for handling colorspace issues properly
            if (auto err = sws_setColorspaceDetails(
                    m_video_stream.sws_ctx, sws_getCoefficients(SWS_CS_DEFAULT), 1,
                    sws_getCoefficients(SWS_CS_ITU709), 0, 0, (1 << 16), (1 << 16));
                err < 0) {
                AKLOG_ERRORN("sws_setColorspaceDetails() failed");
            }

            return true;
        }

        bool FFFrameSink::init_audio_stream() {
            // find codec
            auto codec = avcodec_find_encoder_by_name(m_state->m_encode_conf.audio_codec.c_str());
            if (!codec) {
                AKLOG_ERROR("avcodec_find_encoder_by_name() failed, codec_name: {}",
                            m_state->m_encode_conf.audio_codec.c_str());
                return false;
            }

            // alloc codec ctx
            m_audio_stream.enc_ctx = avcodec_alloc_context3(codec);
            if (!m_audio_stream.enc_ctx) {
                AKLOG_ERRORN("avcodec_alloc_context3() failed");
                return false;
            }
            auto enc_ctx = m_audio_stream.enc_ctx;

            // codec ctx settings
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);

                enc_ctx->sample_rate = m_state->m_atomic_state.encode_audio_spec.load().sample_rate;
                enc_ctx->channel_layout = to_ff_channel_layout(
                    m_state->m_atomic_state.encode_audio_spec.load().channel_layout);
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                enc_ctx->sample_fmt =
                    to_ff_sample_format(m_state->m_atomic_state.encode_audio_spec.load().format);
                enc_ctx->time_base = {1, enc_ctx->sample_rate};
                // [XXX] settings for other params(bit_rate, ...)
            }

            if (m_ofmt_ctx->flags & AVFMT_GLOBALHEADER) {
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            // open encoder
            // codec_opts?
            if (auto err = avcodec_open2(enc_ctx, codec, nullptr); err < 0) {
                AKLOG_ERROR("avcodec_open2() failed, ret={}", av_err2str(err));
                return false;
            }

            // init stream
            m_audio_stream.enc_stream = avformat_new_stream(m_ofmt_ctx, codec);
            if (!m_audio_stream.enc_stream) {
                AKLOG_ERRORN("avformat_new_stream() failed");
                return false;
            }

            if (auto err =
                    avcodec_parameters_from_context(m_audio_stream.enc_stream->codecpar, enc_ctx);
                err < 0) {
                AKLOG_ERROR("avcodec_parameters_from_context() failed, ret={}", av_err2str(err));
                return false;
            }

            return true;
        }

        bool FFFrameSink::init_video_frame(AVFrame** frame, const EncodeArg& encode_arg) {
            // alloc frame
            *frame = av_frame_alloc();
            if (!(*frame)) {
                AKLOG_ERRORN("av_frame_alloc() failed");
                return false;
            }

            // frame settings
            (*frame)->width = m_video_stream.enc_ctx->width;
            (*frame)->height = m_video_stream.enc_ctx->height;
            (*frame)->format = m_video_stream.enc_ctx->pix_fmt;
            // [TODO] settings for interlacing?
            // [TODO] is this really necessary?
            (*frame)->pts = av_rescale_q(encode_arg.pts.num(), {1, (int)encode_arg.pts.den()},
                                         m_video_stream.enc_ctx->time_base);

            // (*frame)->colorspace = m_video_stream.enc_ctx->colorspace;
            (*frame)->colorspace = AVColorSpace::AVCOL_SPC_RGB;
            (*frame)->color_range = AVColorRange::AVCOL_RANGE_JPEG;

            // alloc video buffer
            if (auto err = av_frame_get_buffer(*frame, 0); err < 0) {
                AKLOG_ERROR("av_frame_get_buffer() failed, ret={}", av_err2str(err));
                return false;
            }

            return true;
        }

        bool FFFrameSink::init_audio_frame(AVFrame** frame, const EncodeArg& encode_arg) {
            // alloc frame
            *frame = av_frame_alloc();
            if (!(*frame)) {
                AKLOG_ERRORN("av_frame_alloc() failed");
                return false;
            }

            // frame settings
            (*frame)->nb_samples = encode_arg.nb_samples;
            (*frame)->format = m_audio_stream.enc_ctx->sample_fmt;
            (*frame)->channel_layout = m_audio_stream.enc_ctx->channel_layout;
            // [TODO] is this really necessary?
            (*frame)->pts = av_rescale_q(encode_arg.pts.num(), {1, (int)encode_arg.pts.den()},
                                         m_audio_stream.enc_ctx->time_base);

            // alloc audio buffer
            if (auto err = av_frame_get_buffer(*frame, 0); err < 0) {
                AKLOG_ERROR("av_frame_get_buffer() failed, ret={}", av_err2str(err));
                return false;
            }

            return true;
        }

        bool FFFrameSink::populate_video_frame(AVFrame* frame, const EncodeArg& encode_arg) {
            uint8_t* src_slice[4] = {encode_arg.buffer.get(), 0, 0, 0};
            int src_linesize[4] = {
                av_image_get_linesize(AV_PIX_FMT_RGB24, m_video_stream.enc_ctx->width, 0), 0, 0, 0};
            // clang-format off
            sws_scale(m_video_stream.sws_ctx, 
              // src
              src_slice, src_linesize, 0, m_video_stream.enc_ctx->height, 
              // dst
              frame->data, frame->linesize 
            );
            // clang-format on
            return true;
        }

        bool FFFrameSink::populate_audio_frame(AVFrame* frame, const EncodeArg& encode_arg) {
            if (auto err =
                    avcodec_fill_audio_frame(frame, frame->channels, (AVSampleFormat)frame->format,
                                             encode_arg.buffer.get(), encode_arg.buf_size, 0);
                err < 0) {
                AKLOG_ERROR("avcodec_fill_audio_frame() failed, ret={}", av_err2str(err));
                return false;
            }
            return true;
        }

        void FFFrameSink::flush_encoder(const buffer::AVBufferType& type) {
            AVCodecContext* enc_ctx;
            switch (type) {
                case buffer::AVBufferType::VIDEO: {
                    enc_ctx = m_video_stream.enc_ctx;
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    enc_ctx = m_audio_stream.enc_ctx;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid type found, {}", type);
                    return;
                }
            }

            if (!enc_ctx) {
                return;
            }

            avcodec_send_frame(enc_ctx, nullptr);

            while (true) {
                auto write_result = this->write({type});
                switch (write_result.result) {
                    case codec::EncodeResultCode::ERROR: {
                        AKLOG_ERRORN("Encode error while flushing");
                        return;
                    }
                    case codec::EncodeResultCode::RECV_EOF: {
                        AKLOG_INFON("Successfully flushed the encoder");
                        return;
                    }
                    default: {
                        break;
                    }
                }
            }
        }

    }
}
