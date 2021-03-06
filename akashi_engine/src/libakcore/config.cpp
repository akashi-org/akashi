#include "./config.h"

#include "./rational.h"
#include "./audio.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <tuple>

namespace akashi {
    namespace core {

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Fraction, num, den);

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GenerelConf, entry_file, include_dir);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VideoConf, fps, resolution, default_font_path);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AudioConf, format, sample_rate, channels,
                                           channel_layout);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlaybackConf, enable_loop, gain, decode_method,
                                           video_max_queue_size, video_max_queue_count,
                                           audio_max_queue_size);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UIConf, resolution, window_mode, smart_immersive);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodeConf, out_fname, video_codec, audio_codec,
                                           encode_max_queue_count, encode_method);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AKConf, general, video, audio, playback, ui, encode);

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(AKAudioSampleFormat, {
            {AKAudioSampleFormat::NONE, nullptr},
            {AKAudioSampleFormat::U8, "u8"},
            {AKAudioSampleFormat::S16, "s16"},
            {AKAudioSampleFormat::S32, "s32"},
            {AKAudioSampleFormat::FLT, "flt"},
            {AKAudioSampleFormat::DBL, "dbl"},
            {AKAudioSampleFormat::U8P, "u8p"},
            {AKAudioSampleFormat::S16P, "s16p"},
            {AKAudioSampleFormat::S32P, "s32p"},
            {AKAudioSampleFormat::FLTP, "fltp"},
            {AKAudioSampleFormat::DBLP, "dblp"},
        })
        // clang-format on

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(AKAudioChannelLayout, {
            {AKAudioChannelLayout::NONE, nullptr},
            {AKAudioChannelLayout::MONO, "mono"},
            {AKAudioChannelLayout::STEREO, "stereo"},
        })
        // clang-format on

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(VideoDecodeMethod, {
            {VideoDecodeMethod::NONE, nullptr},
            {VideoDecodeMethod::SW, "sw"},
            {VideoDecodeMethod::VAAPI, "vaapi"},
            {VideoDecodeMethod::VAAPI_COPY, "vaapi_copy"},
        })
        // clang-format on

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(VideoEncodeMethod, {
            {VideoEncodeMethod::NONE, nullptr},
            {VideoEncodeMethod::SW, "sw"},
            {VideoEncodeMethod::VAAPI, "vaapi"}
        })
        // clang-format on

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(WindowMode, {
            {WindowMode::NONE, nullptr},
            {WindowMode::SPLIT, "split"},
            {WindowMode::IMMERSIVE, "immersive"},
            {WindowMode::INDEPENDENT, "independent"}
        })
        // clang-format on

        AKConf parse_akconfig(const char* json_str) {
            auto j = nlohmann::json::parse(json_str);
            return j.get<AKConf>();
        }

    }
}
