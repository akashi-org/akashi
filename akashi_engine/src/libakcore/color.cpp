#include "./color.h"

#include "./rational.h"
#include "./logger.h"

#include <cassert>

namespace akashi {
    namespace core {

        std::array<int, 4> to_rgba_int(std::string color_str) {
            std::array<int, 4> color = {0, 0, 0, 255};

            if (color_str[0] != '#') {
                AKLOG_WARN("Invalid format found: {}", color_str.c_str());
                return {0, 0, 0, 0};
            }

            color_str.erase(0, 1);

            unsigned long value = stoul(color_str, nullptr, 16);

            int offset = 0;
            if (color_str.size() > 6) {
                color[3] = (value >> 0) & 0xff;
                offset = 8;
            }
            color[0] = (value >> (16 + offset)) & 0xff;
            color[1] = (value >> (8 + offset)) & 0xff;
            color[2] = (value >> (0 + offset)) & 0xff;
            return color;
        }

        std::array<float, 4> to_rgba_float(std::string color_str) {
            auto color = to_rgba_int(color_str);

            return {static_cast<float>(core::Rational(color[0], 255).to_decimal()),
                    static_cast<float>(core::Rational(color[1], 255).to_decimal()),
                    static_cast<float>(core::Rational(color[2], 255).to_decimal()),
                    static_cast<float>(core::Rational(color[3], 255).to_decimal())};
        }

    }
}
