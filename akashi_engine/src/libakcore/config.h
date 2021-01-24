#pragma once

#include "./rational.h"
#include "./audio.h"
#include "./hw_accel.h"

#include <string>
#include <vector>
#include <tuple>

namespace akashi {
    namespace core {

        struct GenerelConf {
            std::string entry_file;
            std::string include_dir;
        };

        struct VideoConf {
            Fraction fps;
            std::pair<int, int> resolution;
            std::string default_font_path;
        };

        struct AudioConf : AKAudioSpec {};

        struct PlaybackConf {
            bool enable_loop;
            double gain;
            VideoDecodeMethod decode_method;
            size_t video_max_queue_size;
            size_t video_max_queue_count;
            size_t audio_max_queue_size;
        };

        struct UIConf {
            std::pair<int, int> resolution;
        };

        struct AKConf {
            GenerelConf general;
            VideoConf video;
            AudioConf audio;
            PlaybackConf playback;
            UIConf ui;
        };

        AKConf parse_akconfig(const char* json_str);

    }
}
