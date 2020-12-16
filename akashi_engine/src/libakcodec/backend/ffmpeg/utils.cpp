#include "./utils.h"

#include <libakbuffer/avbuffer.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/samplefmt.h>
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

    }
}
