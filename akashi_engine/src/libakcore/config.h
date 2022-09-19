#pragma once

#include "./rational.h"
#include "./audio.h"
#include "./hw_accel.h"

#include <string>
#include <vector>
#include <tuple>

namespace akashi {
    namespace core {

        struct Fraction {
            int64_t num;
            int64_t den;
        };

        struct GenerelConf {
            std::string entry_file;
            std::string include_dir;
        };

        struct VideoConf {
            Fraction fps;
            std::pair<int, int> resolution;
            std::string default_font_path;
            int msaa;
            std::string vaapi_device;
        };

        struct AudioConf : AKAudioSpec {};

        struct PlaybackConf {
            bool enable_loop;
            double gain;
            VideoDecodeMethod preferred_decode_method;
            size_t video_max_queue_size;
            size_t video_max_queue_count;
            size_t audio_max_queue_size;
        };

        enum class WindowMode { NONE = -1, SPLIT = 0, IMMERSIVE, INDEPENDENT };

        struct UIConf {
            std::pair<int, int> resolution;
            WindowMode window_mode;
            bool smart_immersive;
            bool frameless_window;
        };

        struct EncodeConf {
            std::string out_fname;
            std::string video_codec;
            std::string audio_codec;
            std::string ffmpeg_format_opts;
            std::string video_ffmpeg_codec_opts;
            std::string audio_ffmpeg_codec_opts;
            size_t encode_max_queue_count;
            VideoEncodeMethod encode_method;
        };

        struct AKConf {
            GenerelConf general;
            VideoConf video;
            AudioConf audio;
            PlaybackConf playback;
            UIConf ui;
            EncodeConf encode;
        };

        AKConf parse_akconfig(const char* json_str);

    }
}
