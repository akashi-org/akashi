#include "./buffer.h"

#include "./input.h"
#include "./error.h"

#include <libakbuffer/avbuffer.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
}

using namespace akashi::core;

namespace akashi {
    namespace codec {

        static AVSampleFormat to_ff_sample_format(const AKAudioSampleFormat& format) {
            switch (format) {
                case AKAudioSampleFormat::U8:
                    return AV_SAMPLE_FMT_U8;
                case AKAudioSampleFormat::S16:
                    return AV_SAMPLE_FMT_S16;
                case AKAudioSampleFormat::S32:
                    return AV_SAMPLE_FMT_S32;
                case AKAudioSampleFormat::FLT:
                    return AV_SAMPLE_FMT_FLT;
                case AKAudioSampleFormat::DBL:
                    return AV_SAMPLE_FMT_DBL;
                default:
                    AKLOG_ERROR("to_ff_sample_format() failed. Invalid format {}", format);
                    return AV_SAMPLE_FMT_NONE;
            }
        }

        static int to_ff_channel_layout(const AKAudioChannelLayout& channel_layout) {
            switch (channel_layout) {
                case AKAudioChannelLayout::MONO:
                    return AV_CH_LAYOUT_MONO;
                case AKAudioChannelLayout::STEREO:
                    return AV_CH_LAYOUT_STEREO;
                default: {
                    AKLOG_ERROR("to_ff_channel_layout() failed. Invalid layout {}", channel_layout);
                    return -1;
                }
            }
        }

        FFAudioSpec to_ff_audio_spec(const AKAudioSpec& spec, const int nb_samples) {
            FFAudioSpec ff_spec;

            ff_spec.sample_rate = spec.sample_rate;
            ff_spec.nb_samples = nb_samples;
            ff_spec.format = to_ff_sample_format(spec.format);
            ff_spec.channels = spec.channels;
            ff_spec.channel_layout = to_ff_channel_layout(spec.channel_layout);

            return ff_spec;
        }

        bool to_audio_payload(uint8_t*& out_buf, size_t* out_buf_size, const FFAudioSpec& out_spec,
                              uint8_t* in_buf[AV_NUM_DATA_POINTERS], const FFAudioSpec& in_spec,
                              DecodeStream* dec_stream) {
            int MAX_AUDIO_FRAME_SIZE = 192000; /* 1sec, 48khz, 32bit */
            int64_t in_channel_layout = av_get_default_channel_layout(in_spec.channels);
            uint64_t out_channel_layout = out_spec.channel_layout;
            int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
            AVSampleFormat out_sample_fmt = out_spec.format;
            int out_sample_rate = out_spec.sample_rate;

            int converted_nb_samples = 0;
            int64_t temp_payload_buf_size = 0;

            SwrContext** swr_ctx = &dec_stream->swr_ctx;

            if (!dec_stream->swr_ctx_init_done) {
                *swr_ctx = swr_alloc_set_opts(nullptr, static_cast<int64_t>(out_channel_layout),
                                              out_sample_fmt, out_sample_rate, in_channel_layout,
                                              (AVSampleFormat)(in_spec.format), in_spec.sample_rate,
                                              1, nullptr);
            }

            if (*swr_ctx == nullptr) {
                AKLOG_ERRORN("to_audio_payload(): failed to allocate swr context");
                return false;
            }

            if (!dec_stream->swr_ctx_init_done) {
                int ff_ret = swr_init(*swr_ctx);
                if (ff_ret < 0) {
                    AKLOG_ERROR("to_audio_payload(): failed to init swr: ret={:08x}",
                                AVERROR(ff_ret));
                    return false;
                }
                dec_stream->swr_ctx_init_done = true;
            }

            out_buf = (uint8_t*)av_malloc(static_cast<size_t>(MAX_AUDIO_FRAME_SIZE) * 2);
            if (!out_buf) {
                AKLOG_ERRORN("to_audio_payload(): failed to allocate audio buf");
                return false;
            }

            if ((converted_nb_samples =
                     swr_convert(*swr_ctx, &out_buf, MAX_AUDIO_FRAME_SIZE,
                                 (const uint8_t**)(in_buf), in_spec.nb_samples)) < 0) {
                AKLOG_ERRORN("to_audio_payload(): failed to resample");
                return false;
            }

            temp_payload_buf_size = av_samples_get_buffer_size(
                nullptr, out_channels, converted_nb_samples, out_sample_fmt, 1);
            if (temp_payload_buf_size < 0) {
                AKLOG_ERRORN("to_audio_payload(): failed to get audio buf size");
                return false;
            }
            *out_buf_size = temp_payload_buf_size;

            return true;
        }

        FFmpegBufferData::FFmpegBufferData(const InputData& input, DecodeStream* dec_stream) {
            FFmpegBufferData::Property prop;

            prop.media_type = input.media_type;
            prop.pts = input.pts;
            prop.rpts = input.rpts;
            prop.uuid = input.uuid;
            prop.start_frame = input.start_frame;
            prop.decode_method = input.decode_method;

            m_prop = prop;

            memcpy(this->m_linesize, input.frame->linesize, sizeof(this->m_linesize));

            switch (m_prop.media_type) {
                case buffer::AVBufferType::VIDEO: {
                    this->populate_video(input.frame);
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    this->populate_audio(input.frame, input.out_audio_spec, dec_stream);
                    break;
                }
                default: {
                }
            }
        }

        FFmpegBufferData::~FFmpegBufferData() {
            switch (m_prop.media_type) {
                case buffer::AVBufferType::VIDEO: {
                    for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
                        if (m_linesize[i] > 0 && m_prop.video_data[i].buf != nullptr) {
                            av_free(m_prop.video_data[i].buf);
                            m_prop.video_data[i].buf = nullptr;
                        }
                    }
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    av_free(m_prop.audio_data);
                    m_prop.audio_data = nullptr;
                    break;
                }
                default: {
                    AKLOG_ERROR("FFmpegBufferData::~FFmpegBufferData(): invalid media type {}",
                                m_prop.media_type);
                }
            }
        }

        void FFmpegBufferData::populate_video(AVFrame* frame) {
            m_prop.width = frame->width;
            m_prop.height = frame->height;

            const AVPixFmtDescriptor* desc =
                av_pix_fmt_desc_get(static_cast<AVPixelFormat>(frame->format));
            if (!desc) {
                AKLOG_ERRORN("FFmpegBufferData::popludate_video(): Failed to get descriptor");
            }
            m_prop.chroma_width = frame->width >> desc->log2_chroma_w;
            m_prop.chroma_height = frame->height >> desc->log2_chroma_h;
            int plain_height[] = {frame->height, m_prop.chroma_height, m_prop.chroma_height};

            for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) {
                if (frame->linesize[i] == 0) {
                    break;
                }

                FFmpegBufferData::Property::VideoEntry entry;
                entry.stride = frame->linesize[i];

                auto plain_buf_size = frame->linesize[i] * plain_height[i];
                entry.buf = static_cast<uint8_t*>(av_malloc(plain_buf_size));
                if (!entry.buf) {
                    AKLOG_ERRORN("FFmpegBufferData::popludate_video(): Failed to alloc data");
                    continue;
                }
                std::memcpy(entry.buf, static_cast<const uint8_t*>(frame->data[i]), plain_buf_size);

                m_prop.video_data[i] = entry;
                m_prop.data_size += plain_buf_size;
            }
        }

        void FFmpegBufferData::populate_audio(const AVFrame* frame,
                                              const AKAudioSpec& out_audio_spec,
                                              DecodeStream* dec_stream) {
            uint8_t* in_buf[AV_NUM_DATA_POINTERS];
            FFAudioSpec in_spec;
            FFAudioSpec out_spec = to_ff_audio_spec(out_audio_spec, frame->nb_samples);

            in_spec.channels = frame->channels;
            in_spec.channel_layout = frame->channel_layout;
            in_spec.format = static_cast<AVSampleFormat>(frame->format);
            in_spec.nb_samples = frame->nb_samples;
            in_spec.sample_rate = frame->sample_rate;

            // [TODO] is malloc really necessary for in_buf?

            // a case where the format is packed
            if (in_spec.format < AV_SAMPLE_FMT_U8P) {
                // [TODO] why not simply use `in_buf[0] = frame.extended_data[0]`?
                int size_ = av_samples_get_buffer_size(nullptr, frame->channels, frame->nb_samples,
                                                       in_spec.format, 1);

                auto plain_buf_size = size_;
                in_buf[0] = static_cast<uint8_t*>(av_malloc(static_cast<size_t>(plain_buf_size)));
                if (!in_buf[0]) {
                    AKLOG_ERRORN("FFmpegBufferData::populate_audio(): Failed to alloc data");
                }
                memcpy(in_buf[0], static_cast<const uint8_t*>(frame->extended_data[0]),
                       plain_buf_size);
            }
            // a case where the format is planar
            else {
                for (int i = 0; i < in_spec.channels; i++) {
                    int size_ = av_samples_get_buffer_size(nullptr, in_spec.channels,
                                                           in_spec.nb_samples, in_spec.format, 1);

                    auto plain_buf_size = size_ / in_spec.channels;
                    in_buf[i] =
                        static_cast<uint8_t*>(av_malloc(static_cast<size_t>(plain_buf_size)));
                    if (!in_buf[i]) {
                        AKLOG_ERRORN("FFmpegBufferData::populate_audio(): Failed to alloc data");
                        continue;
                    }
                    memcpy(in_buf[i], static_cast<const uint8_t*>(frame->extended_data[i]),
                           plain_buf_size);
                    // this->data_size += this->plain_buf_size[i];
                }
            }

            if (!to_audio_payload(m_prop.audio_data, &m_prop.data_size, out_spec, in_buf, in_spec,
                                  dec_stream)) {
                AKLOG_ERRORN("FFmpegBufferData::populate_audio(): Failed to convert audio data");
            }

            m_prop.sample_rate = out_audio_spec.sample_rate;
            m_prop.sample_format = out_audio_spec.format;
            m_prop.channels = out_audio_spec.channels;
            m_prop.nb_samples = out_spec.nb_samples;

            if (in_spec.format < AV_SAMPLE_FMT_U8P) {
                av_free(in_buf[0]);
                in_buf[0] = nullptr;
            } else {
                for (int i = 0; i < in_spec.channels; i++) {
                    if (in_buf[i] != nullptr) {
                        av_free(in_buf[i]);
                        in_buf[i] = nullptr;
                    }
                }
            }
        }
    }
}
