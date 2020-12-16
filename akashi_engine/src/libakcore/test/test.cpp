#include <catch.hpp>
#include "../rational.h"

using namespace akashi::core;

namespace akashi {
    namespace core {

        TEST_CASE("rational test", "[akcore]") {
            REQUIRE(Rational(1, 3) + Rational(2, 3) == Rational(1, 1));
            REQUIRE(Rational(1, 3) * Rational(2, 3) == Rational(2, 9));

            REQUIRE_THROWS_WITH(Rational(10, 0), "Rational Exception:: den must not be zero");
        }

        // TEST_CASE("benchmark", "[akcore/bench]") {
        //     BENCHMARK("Fibonacci 20") { return Rational(1, 2); };
        // }

    }

}
