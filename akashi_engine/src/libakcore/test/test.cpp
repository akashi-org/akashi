#include <catch.hpp>
#include "../rational.h"
#include "../time.h"

using namespace akashi::core;

namespace akashi {
    namespace core {

        TEST_CASE("rational test", "[akcore]") {
            REQUIRE(Rational(1, 3) + Rational(2, 3) == Rational(1, 1));
            REQUIRE(Rational(1, 3) * Rational(2, 3) == Rational(2, 9));

            REQUIRE_THROWS_WITH(Rational(10, 0), "Rational Exception:: den must not be zero");
        }

        TEST_CASE("time test", "[akcore]") {
            REQUIRE(to_time_string(30) == "00:00:00.030");
            REQUIRE(to_time_string((Rational(60 + 18, 1).to_decimal() * 1000) + 129) ==
                    "00:01:18.129");
        }

        // TEST_CASE("benchmark", "[akcore/bench]") {
        //     BENCHMARK("Fibonacci 20") { return Rational(1, 2); };
        // }

    }

}
