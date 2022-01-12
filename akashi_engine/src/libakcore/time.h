#pragma once

#include <string>

namespace akashi {

    namespace core {

        class Rational;

        // hh::mm::ss.zzz
        std::string to_time_string(long time_ms, bool format_msec = true);
    }
}
