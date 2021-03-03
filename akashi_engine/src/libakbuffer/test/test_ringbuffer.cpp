#include <catch.hpp>
#include "../ringbuffer.h"

namespace akashi {
    namespace buffer {

        TEST_CASE("AudioRingBuffer:: no mix case") {
            AudioRingBuffer abuffer{1024 * 4, {}};

            const auto& raw = abuffer.raw();

            float wbuf[500] = {1.0};
            for (int i = 0; i < 500; i++) {
                wbuf[i] = 1.0;
            }
            REQUIRE(abuffer.write(wbuf, 500, core::Rational(0, 1)));
            REQUIRE(abuffer.write(wbuf, 500, abuffer.to_pts(500)));

            REQUIRE(abuffer.seek(500));

            float wbuf2[500] = {2.0};
            for (int i = 0; i < 500; i++) {
                wbuf2[i] = 2.0;
            }
            REQUIRE(abuffer.write(wbuf2, 500, abuffer.to_pts(1000)));

            REQUIRE(raw[0] == 2.0);
            REQUIRE(raw[475] == 2.0);

            REQUIRE(raw[476] == 1.0);
            REQUIRE(raw[500] == 1.0);
            REQUIRE(raw[999] == 1.0);

            REQUIRE(raw[1000] == 2.0);
            REQUIRE(raw[1023] == 2.0);

            REQUIRE(abuffer.buf_pts().num() == abuffer.to_pts(500).num());
            REQUIRE(abuffer.buf_pts().den() == abuffer.to_pts(500).den());
        }

        TEST_CASE("AudioRingBuffer:: mix case") {
            AudioRingBuffer abuffer{1024 * 4, {}};

            const auto& raw = abuffer.raw();

            float wbuf[500] = {1.0};
            for (int i = 0; i < 500; i++) {
                wbuf[i] = 1.0;
            }
            REQUIRE(abuffer.write(wbuf, 500, core::Rational(0, 1)));
            REQUIRE(abuffer.write(wbuf, 500, abuffer.to_pts(500)));

            REQUIRE(abuffer.seek(500));

            float wbuf2[500] = {2.0};
            for (int i = 0; i < 500; i++) {
                wbuf2[i] = 2.0;
            }
            REQUIRE(abuffer.write(wbuf2, 500, abuffer.to_pts(1000), true));

            REQUIRE(raw[0] == 3.0);
            REQUIRE(raw[475] == 3.0);

            REQUIRE(raw[476] == 1.0);
            REQUIRE(raw[500] == 1.0);
            REQUIRE(raw[999] == 1.0);

            REQUIRE(raw[1000] == 2.0);
            REQUIRE(raw[1023] == 2.0);

            REQUIRE(abuffer.buf_pts().num() == abuffer.to_pts(500).num());
            REQUIRE(abuffer.buf_pts().den() == abuffer.to_pts(500).den());
        }
    }
}
