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
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UIConf, resolution);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodeConf, out_fname, video_codec, audio_codec);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AKConf, general, video, audio, playback, ui, encode);

        AKConf parse_akconfig(const char* json_str) {
            auto j = nlohmann::json::parse(json_str);
            return j.get<AKConf>();
        }

    }
}
