#pragma once

#include <libakcore/rational.h>

extern "C" {
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/codec_id.h>
}

namespace akashi {
    namespace buffer {
        enum class AVBufferType;
    }
    namespace core {
        enum class EncodeCodec;
        enum class AKAudioChannelLayout;
        enum class AKAudioSampleFormat;
    }
    namespace codec {

        akashi::core::Rational to_rational(const AVRational& av_r);

        AVRational to_av_rational(const akashi::core::Rational& rat);

        AVMediaType to_ff_media_type(const buffer::AVBufferType* media_type);

        buffer::AVBufferType to_res_buf_type(const AVMediaType& media_type);

        AVCodecID to_ff_codec_id(const core::EncodeCodec& codec);

        uint64_t to_ff_channel_layout(const core::AKAudioChannelLayout& channel_layout);

        AVSampleFormat to_ff_sample_format(const core::AKAudioSampleFormat& format,
                                           bool force_planar = false);

    }
}
