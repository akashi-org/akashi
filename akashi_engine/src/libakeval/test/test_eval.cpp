#include <catch.hpp>

#include "../akeval.h"

#include <libakstate/akstate.h>
#include <libakcore/memory.h>

#include <cstdio>
#include <iostream>

namespace akashi {
    namespace codec {

        TEST_CASE("test1", "[akeval]") {
            auto akconf = akashi::core::parse_akconfig(std::getenv("AK_TEST_CONFIG_STR"));
            akashi::state::AKState state(akconf, std::getenv("AK_TEST_CONFIG_PATH"));
            eval::AKEval eval{core::borrowed_ptr(&state)};

            printf("Press any key to continue.\n");
            std::string j;
            std::cin >> j;

            const auto& render_prof = eval.render_prof(std::getenv("AK_TEST_ENTRY_FPATH"), "");

            // REQUIRE(1 == 1);
        }

    }
}
