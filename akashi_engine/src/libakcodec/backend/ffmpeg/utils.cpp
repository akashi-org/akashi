#include "./utils.h"

#include <libakbuffer/avbuffer.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakcore/audio.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
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
                    AKLOG_ERROR("Invalid type {}", *media_type);
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
                    AKLOG_ERROR("Invalid type {}", media_type);
                    return buffer::AVBufferType::UNKNOWN;
            }
        }

        uint64_t to_ff_channel_layout(const core::AKAudioChannelLayout& channel_layout) {
            switch (channel_layout) {
                case core::AKAudioChannelLayout::MONO:
                    return AV_CH_LAYOUT_MONO;
                case core::AKAudioChannelLayout::STEREO:
                    return AV_CH_LAYOUT_STEREO;
                default: {
                    AKLOG_ERROR("Invalid layout {}", channel_layout);
                    return -1;
                }
            }
        }

        AVSampleFormat to_ff_sample_format(const core::AKAudioSampleFormat& format) {
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
                case AKAudioSampleFormat::U8P:
                    return AV_SAMPLE_FMT_U8P;
                case AKAudioSampleFormat::S16P:
                    return AV_SAMPLE_FMT_S16P;
                case AKAudioSampleFormat::S32P:
                    return AV_SAMPLE_FMT_S32P;
                case AKAudioSampleFormat::FLTP:
                    return AV_SAMPLE_FMT_FLTP;
                case AKAudioSampleFormat::DBLP:
                    return AV_SAMPLE_FMT_DBLP;
                default:
                    AKLOG_ERROR("Invalid format {}", format);
                    return AV_SAMPLE_FMT_NONE;
            }
        }

        core::AKAudioSampleFormat from_ff_sample_format(const AVSampleFormat& format) {
            switch (format) {
                case AV_SAMPLE_FMT_U8:
                    return AKAudioSampleFormat::U8;
                case AV_SAMPLE_FMT_S16:
                    return AKAudioSampleFormat::S16;
                case AV_SAMPLE_FMT_S32:
                    return AKAudioSampleFormat::S32;
                case AV_SAMPLE_FMT_FLT:
                    return AKAudioSampleFormat::FLT;
                case AV_SAMPLE_FMT_DBL:
                    return AKAudioSampleFormat::DBL;
                case AV_SAMPLE_FMT_U8P:
                    return AKAudioSampleFormat::U8P;
                case AV_SAMPLE_FMT_S16P:
                    return AKAudioSampleFormat::S16P;
                case AV_SAMPLE_FMT_S32P:
                    return AKAudioSampleFormat::S32P;
                case AV_SAMPLE_FMT_FLTP:
                    return AKAudioSampleFormat::FLTP;
                case AV_SAMPLE_FMT_DBLP:
                    return AKAudioSampleFormat::DBLP;
                default:
                    AKLOG_ERROR("Invalid format {}", format);
                    return AKAudioSampleFormat::NONE;
            }
        }

        bool is_hw_pix_fmt(const AVPixelFormat& format) {
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(format);
            return desc->flags & AV_PIX_FMT_FLAG_HWACCEL;
        }

    }
}
