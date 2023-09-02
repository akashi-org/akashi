#include <catch.hpp>

#include "../../../backend/pulseaudio/callback.h"
#include "../../../backend/pulseaudio/util.h"
#include "../../../backend/pulseaudio/etc.h"

#include <libakcore/element.h>
#include <libakcore/audio.h>
#include <libakcore/rational.h>

#include <cstring>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        namespace priv {

            static size_t pa_bytes_per_second(const core::AKAudioSpec& audio_spec) {
                pa_sample_spec spec;
                spec.rate = audio_spec.sample_rate;
                spec.format = to_pl_sample_format(audio_spec.format).value;
                spec.channels = audio_spec.channels;
                return pa_bytes_per_second(&spec);
            }
        }

        TEST_CASE("dummy", "[akaudio]") {}

        // TEST_CASE("collect_segments_no_loop", "[akaudio]") {
        //     uint8_t mask_buf[MAX_AUDIO_BUFFER_SIZE] = {0};
        //     size_t requested_bytes = 1024 * 4;

        //     memset(mask_buf, 0, requested_bytes);

        //     core::AKAudioSpec audio_spec = {.format = AKAudioSampleFormat::FLT,
        //                                     .sample_rate = 44100,
        //                                     .channels = 2,
        //                                     .channel_layout =
        //                                     core::AKAudioChannelLayout::STEREO};

        //     auto bytes_per_second = priv::pa_bytes_per_second(audio_spec);
        //     AudioProfile audio_profile = {
        //         .current_time = bytes_to_dur(bytes_per_second, 1024) * 1024,
        //         // 2.9721541950113379 .bytes_per_second = bytes_per_second, .duration =
        //         bytes_to_dur(bytes_per_second, 1024) * 1024 * 4 // 11.888616780045352
        //     };

        //     std::vector<WBSegment> segments;

        //     auto r = collect_segments(&segments, audio_profile.current_time, audio_profile,
        //                               mask_buf, requested_bytes);

        //     REQUIRE(r);
        //     REQUIRE(segments.size() == 1);
        //     REQUIRE(&segments[0].buf[0] == &mask_buf[0]);
        //     REQUIRE(segments[0].from_pts == audio_profile.current_time);
        //     REQUIRE(segments[0].to_pts ==
        //             audio_profile.current_time + bytes_to_dur(bytes_per_second,
        //             requested_bytes));
        //     REQUIRE(segments[0].buf_size == requested_bytes);
        // }

        // TEST_CASE("collect_segments_diff_loop", "[akaudio]") {
        //     uint8_t mask_buf[MAX_AUDIO_BUFFER_SIZE] = {0};
        //     size_t requested_bytes = 1024 * 8;

        //     memset(mask_buf, 0, requested_bytes);

        //     core::AKAudioSpec audio_spec = {.format = AKAudioSampleFormat::FLT,
        //                                     .sample_rate = 44100,
        //                                     .channels = 2,
        //                                     .channel_layout =
        //                                     core::AKAudioChannelLayout::STEREO};

        //     auto bytes_per_second = priv::pa_bytes_per_second(audio_spec);
        //     AudioProfile audio_profile = {
        //         .current_time = (bytes_to_dur(bytes_per_second, 1024) * 1024),
        //         // 2.9721541950113379 .bytes_per_second = bytes_per_second, .duration =
        //         core::Rational(2.98)};

        //     size_t loop_cnt = 0;

        //     std::vector<WBSegment> segments;

        //     auto r = collect_segments(&segments, audio_profile.current_time, audio_profile,
        //                               mask_buf, requested_bytes, loop_cnt);

        //     REQUIRE(r);
        //     REQUIRE(segments.size() == 2);
        //     REQUIRE((segments[0].buf_size + segments[1].buf_size) == requested_bytes);
        //     REQUIRE(&segments[0].buf[0] == &mask_buf[0]);
        //     REQUIRE(&segments[1].buf[0] == &mask_buf[segments[0].buf_size]);
        //     REQUIRE(segments[0].loop_cnt == 0);
        //     REQUIRE(segments[1].loop_cnt == 1);
        //     REQUIRE(segments[0].from_pts == audio_profile.current_time);
        //     REQUIRE(segments[0].to_pts == audio_profile.duration);
        //     REQUIRE(segments[1].from_pts == Rational(0l));
        //     REQUIRE(segments[1].to_pts == bytes_to_dur(bytes_per_second, segments[1].buf_size));
        // }

        // TEST_CASE("collect_segments_no_diff_loop", "[akaudio]") {
        //     uint8_t mask_buf[MAX_AUDIO_BUFFER_SIZE] = {0};
        //     size_t requested_bytes = 1024 * 4;

        //     memset(mask_buf, 0, requested_bytes);

        //     core::AKAudioSpec audio_spec = {.format = AKAudioSampleFormat::FLT,
        //                                     .sample_rate = 44100,
        //                                     .channels = 2,
        //                                     .channel_layout =
        //                                     core::AKAudioChannelLayout::STEREO};

        //     auto bytes_per_second = priv::pa_bytes_per_second(audio_spec);
        //     AudioProfile audio_profile = {
        //         .current_time =
        //             bytes_to_dur(bytes_per_second, 1024) * 1024 * 4, // 11.888616780045352
        //         .bytes_per_second = bytes_per_second,
        //         .duration = bytes_to_dur(bytes_per_second, 1024) * 1024 * 4 // 11.888616780045352
        //     };

        //     size_t loop_cnt = 0;

        //     std::vector<WBSegment> segments;

        //     auto r = collect_segments(&segments, audio_profile.current_time, audio_profile,
        //                               mask_buf, requested_bytes, loop_cnt);

        //     REQUIRE(r);
        //     REQUIRE(segments.size() == 1);
        //     REQUIRE(&segments[0].buf[0] == &mask_buf[0]);
        //     REQUIRE(segments[0].from_pts == core::Rational(0l));
        //     REQUIRE(segments[0].to_pts == bytes_to_dur(bytes_per_second, requested_bytes));
        //     REQUIRE(segments[0].buf_size == requested_bytes);
        //     REQUIRE(segments[0].loop_cnt == 1);
        // }

    }
}
