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
                {.from = {.num = 0, .den = 1},
                 .to = {.num = 10, .den = 1},
                 .duration = {.num = 10, .den = 1},
                 .uuid = "0c5f3d5f-56b2-4977-b799-20115f72dbd7",
                 .layers = {
                     {.type = LayerType::VIDEO,
                      .from = {.num = 0, .den = 1},
                      .to = {.num = 7, .den = 1},
                      .uuid = "c01c77fc-44b0-455d-b91f-870b92212693",
                      .src = "../src/libakcodec/test/fixtures/countdown1/countdown1_720p.mp4",
                      .start = {.num = 0, .den = 1}},
                     {.type = LayerType::VIDEO,
                      .from = {.num = 2, .den = 1},
                      .to = {.num = 10, .den = 1},
                      .uuid = "d5ed6e83-1b88-4f62-92e3-75b777f1de8c",
                      .src = "../src/libakcodec/test/fixtures/countdown1/countdown1_720p.mp4",
                      .start = {.num = 0, .den = 1}},
                 }});
            atom_profiles.push_back(
                {.from = {.num = 241, .den = 24}, // 10.041666666666666
                 .to = {.num = 721, .den = 24},   // 30.041666666666668
                 .duration = {.num = 20, .den = 1},
                 .uuid = "3de58572-9d86-41a6-9f24-c957a3fef2cc",
                 .layers = {
                     {.type = LayerType::VIDEO,
                      .from = {.num = 12, .den = 1},
                      .to = {.num = 17, .den = 1},
                      .uuid = "61094793-de40-42c4-af47-eaca4c1b80c3",
                      .src = "../src/libakcodec/test/fixtures/countdown1/countdown1_720p.mp4",
                      .start = {.num = 0, .den = 1}}

                 }});

            AKAudioSpec audio_spec;
            Rational decode_start{0, 1};

            auto decoder = new AKDecoder(atom_profiles, decode_start);

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
                            decoder = new AKDecoder(atom_profiles, decode_start);
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
                                static_cast<int>(decode_res.result), decode_res.layer_uuid);
                        continue;
                    }
                    case DecodeResultCode::OK: {
                        switch (decode_res.buffer->prop().media_type) {
                            case buffer::AVBufferType::VIDEO: {
                                printf("v: %f, uuid: %s\n",
                                       decode_res.buffer->prop().pts.to_decimal(),
                                       decode_res.layer_uuid);
                                break;
                            }
                            case buffer::AVBufferType::AUDIO: {
                                printf("a: %f, uuid: %s\n",
                                       decode_res.buffer->prop().pts.to_decimal(),
                                       decode_res.layer_uuid);
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
