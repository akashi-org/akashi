#pragma once

#include <cstddef>
#include <cstdint>

namespace akashi {
    namespace core {

        // [TODO] endian? 24bit?
        enum class AKAudioSampleFormat {
            NONE = -1,

            // interleaved format
            U8 = 0,
            S16,
            S32,
            FLT,
            DBL,

            // planar format
            U8P,
            S16P,
            S32P,
            FLTP,
            DBLP
        };

        enum class AKAudioChannelLayout { NONE = -1, MONO = 0, STEREO };

        struct AKAudioSpec {
            AKAudioSampleFormat format = AKAudioSampleFormat::FLT;
            int sample_rate = 44100;
            int channels = 2;
            AKAudioChannelLayout channel_layout = AKAudioChannelLayout::STEREO;
        };

        inline int size_table(AKAudioSampleFormat format) {
            switch (format) {
                case AKAudioSampleFormat::U8:
                case AKAudioSampleFormat::U8P:
                    return 1;
                case AKAudioSampleFormat::S16:
                case AKAudioSampleFormat::S16P:
                    return 2;
                case AKAudioSampleFormat::S32:
                case AKAudioSampleFormat::S32P:
                    return 4;
                case AKAudioSampleFormat::FLT:
                case AKAudioSampleFormat::FLTP:
                    return 4;
                case AKAudioSampleFormat::DBL:
                case AKAudioSampleFormat::DBLP:
                    return 8;
                default:
                    return 1;
            }
        }

        inline int64_t bytes_per_second(const AKAudioSpec& spec) {
            return spec.sample_rate * size_table(spec.format) * spec.channels;
        }

    }
}
