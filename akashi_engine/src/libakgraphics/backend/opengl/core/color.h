#pragma once

#include <string>
#include <array>

namespace akashi {
    namespace graphics {

        std::array<int, 4> to_rgba_int(std::string color_str);

        std::array<double, 4> to_rgba_double(std::string color_str);

    }
}
