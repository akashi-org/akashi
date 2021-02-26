#include "./utils.h"

#include <libakcore/codec.h>
#include <libakbuffer/avbuffer.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakcore/audio.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}

using namespace akashi::core;

namespace akashi {
    namespace codec {

        akashi::core::Rational to_rational(const AVRational& av_r) {
            return akashi::core::Rational(av_r.num, av_r.den);
        }

        AVRational to_av_rational(const akashi::core::Rational& rat) {
            // [TODO] is it ok to convert int64_t -> int?
            return {.num = static_cast<int>(rat.num()), .den = static_cast<int>(rat.den())};
        }

        AVMediaType to_ff_media_type(const buffer::AVBufferType* media_type) {
            switch (*media_type) {
                case buffer::AVBufferType::VIDEO:
                    return AVMEDIA_TYPE_VIDEO;
                case buffer::AVBufferType::AUDIO:
                    return AVMEDIA_TYPE_AUDIO;
                default:
                    AKLOG_ERROR("to_ff_media_type() failed. Invalid type {}", *media_type);
                    return AVMEDIA_TYPE_UNKNOWN;
            }
        }

        buffer::AVBufferType to_res_buf_type(const AVMediaType& media_type) {
            switch (media_type) {
                case AVMEDIA_TYPE_VIDEO:
                    return buffer::AVBufferType::VIDEO;
                case AVMEDIA_TYPE_AUDIO:
                    return buffer::AVBufferType::AUDIO;
                default:
                    AKLOG_ERROR("to_res_buf_type() failed. Invalid type {}", media_type);
                    return buffer::AVBufferType::UNKNOWN;
            }
        }

        AVCodecID to_ff_codec_id(const core::EncodeCodec& codec) {
            switch (codec) {
                case EncodeCodec::V_H264:
                    return AV_CODEC_ID_H264;
                case EncodeCodec::A_AAC:
                    return AV_CODEC_ID_AAC;
                case EncodeCodec::A_MP3:
                    return AV_CODEC_ID_MP3;
                default: {
                    return AV_CODEC_ID_NONE;
                }
            }
        }

        uint64_t to_ff_channel_layout(const core::AKAudioChannelLayout& channel_layout) {
            switch (channel_layout) {
                case core::AKAudioChannelLayout::MONO:
                    return AV_CH_LAYOUT_MONO;
                case core::AKAudioChannelLayout::STEREO:
                    return AV_CH_LAYOUT_STEREO;
                case core::AKAudioChannelLayout::NONE: {
                    AKLOG_ERROR("to_ff_channel_layout() failed. Invalid layout {}", channel_layout);
                    return -1;
                }
            }
        }

        AVSampleFormat to_ff_sample_format(const core::AKAudioSampleFormat& format,
                                           bool force_planar) {
            switch (format) {
                case AKAudioSampleFormat::U8:
                    return force_planar ? AV_SAMPLE_FMT_U8P : AV_SAMPLE_FMT_U8;
                case AKAudioSampleFormat::S16:
                    return force_planar ? AV_SAMPLE_FMT_S16P : AV_SAMPLE_FMT_S16;
                case AKAudioSampleFormat::S32:
                    return force_planar ? AV_SAMPLE_FMT_S32P : AV_SAMPLE_FMT_S32;
                case AKAudioSampleFormat::FLT:
                    return force_planar ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_FLT;
                case AKAudioSampleFormat::DBL:
                    return force_planar ? AV_SAMPLE_FMT_DBLP : AV_SAMPLE_FMT_DBL;
                default:
                    AKLOG_ERROR("to_ff_sample_format() failed. Invalid format {}", format);
                    return AV_SAMPLE_FMT_NONE;
            }
        }

    }
}
