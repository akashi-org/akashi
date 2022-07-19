#pragma once

#include <string>
#include <array>

namespace akashi {
    namespace core {

        std::array<int, 4> to_rgba_int(std::string color_str);

        std::array<float, 4> to_rgba_float(std::string color_str);

    }
}
