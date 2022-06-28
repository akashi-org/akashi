#include <catch.hpp>

#include "../../../decode_item.h"
#include "../../../decoder.h"

#include <libakcore/element.h>
#include <libakcore/audio.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/hw_accel.h>

#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>
#include <libakbuffer/video_queue.h>

#include <filesystem>

using namespace akashi::core;

namespace akashi {
    namespace codec {

        TEST_CASE("test1", "[akcodec]") {
            std::vector<AtomProfile> atom_profiles;

            printf("%s\n", std::filesystem::current_path().c_str());

            atom_profiles.push_back(
                {.from = Rational{0, 1},
                 .to = Rational{10, 1},
                 .duration = Rational{10, 1},
                 .uuid = "0c5f3d5f-56b2-4977-b799-20115f72dbd7",
                 .av_layers = {
                     {.type = LayerType::VIDEO,
                      .from = Rational{0, 1},
                      .to = Rational{7, 1},
                      .uuid = "c01c77fc-44b0-455d-b91f-870b92212693",
                      .src = "../src/libakcodec/test/fixtures/countdown1/countdown1_720p.mp4",
                      .start = Rational{0, 1}},
                     {.type = LayerType::VIDEO,
                      .from = Rational{2, 1},
                      .to = Rational{10, 1},
                      .uuid = "d5ed6e83-1b88-4f62-92e3-75b777f1de8c",
                      .src = "../src/libakcodec/test/fixtures/countdown1/countdown1_720p.mp4",
                      .start = Rational{0, 1}},
                 }});
            atom_profiles.push_back(
                {.from = Rational{241, 24}, // 10.041666666666666
                 .to = Rational{721, 24},   // 30.041666666666668
                 .duration = Rational{20, 1},
                 .uuid = "3de58572-9d86-41a6-9f24-c957a3fef2cc",
                 .av_layers = {
                     {.type = LayerType::VIDEO,
                      .from = Rational{12, 1},
                      .to = Rational{17, 1},
                      .uuid = "61094793-de40-42c4-af47-eaca4c1b80c3",
                      .src = "../src/libakcodec/test/fixtures/countdown1/countdown1_720p.mp4",
                      .start = Rational{0, 1}}

                 }});

            core::RenderProfile render_prof;

            render_prof.atom_profiles = atom_profiles;
            core::Rational global_duration;
            for (const auto& atom_prof : atom_profiles) {
                global_duration += atom_prof.duration;
            }
            render_prof.duration = global_duration;

            AKAudioSpec audio_spec;
            Rational decode_start{0, 1};

            auto decoder = new AKDecoder(render_prof, decode_start);

            bool enable_loop = false;
            bool decode_finished = false;
            while (!decode_finished) {
                auto decode_res = decoder->decode({audio_spec, core::VideoDecodeMethod::SW});

                switch (decode_res.result) {
                    case DecodeResultCode::ERROR: {
                        fprintf(stderr, "decode error, code: %d\n",
                                static_cast<int>(decode_res.result));
                        decode_finished = true;
                        break;
                    }
                    case DecodeResultCode::DECODE_ENDED: {
                        fprintf(stderr, "ended, code: %d\n", static_cast<int>(decode_res.result));
                        if (enable_loop) {
                            delete decoder;
                            decoder = new AKDecoder(render_prof, decode_start);
                        } else {
                            decode_finished = true;
                        }
                        break;
                    }
                    case DecodeResultCode::DECODE_LAYER_EOF:
                    case DecodeResultCode::DECODE_LAYER_ENDED:
                    case DecodeResultCode::DECODE_STREAM_ENDED:
                    case DecodeResultCode::DECODE_ATOM_ENDED:
                    case DecodeResultCode::DECODE_AGAIN:
                    case DecodeResultCode::DECODE_SKIPPED: {
                        fprintf(stderr, "decode skipped or layer ended, code: %d, uuid: %s\n",
                                static_cast<int>(decode_res.result), decode_res.layer_uuid.c_str());
                        continue;
                    }
                    case DecodeResultCode::OK: {
                        switch (decode_res.buffer->prop().media_type) {
                            case buffer::AVBufferType::VIDEO: {
                                printf("v: %f, uuid: %s\n",
                                       decode_res.buffer->prop().pts.to_decimal(),
                                       decode_res.layer_uuid.c_str());
                                break;
                            }
                            case buffer::AVBufferType::AUDIO: {
                                printf("a: %f, uuid: %s\n",
                                       decode_res.buffer->prop().pts.to_decimal(),
                                       decode_res.layer_uuid.c_str());
                                break;
                            }
                            default: {
                            }
                        }
                        break;
                    }
                    default: {
                        fprintf(stderr, "invalid result code found, code: %d\n",
                                static_cast<int>(decode_res.result));
                        decode_finished = true;
                        break;
                    }
                }
            }

            REQUIRE(1 == 1);
        }

    }
}
