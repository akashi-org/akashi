#include "./elem.h"

#include <libakcore/logger.h>

#include <pybind11/embed.h>
#include <string>
#include <stdexcept>

namespace akashi {
    namespace eval {

        ElementType elem_type(const pybind11::object& elem) noexcept(false) {
            std::string elem_str = elem().cast<std::string>();
            if (elem_str == "ROOT") {
                return ElementType::ROOT;
            } else if (elem_str == "SCENE") {
                return ElementType::SCENE;
            } else if (elem_str == "ATOM") {
                return ElementType::ATOM;
            } else if (elem_str == "LAYER" || elem_str == "VIDEO" || elem_str == "AUDIO" ||
                       elem_str == "IMAGE" || elem_str == "TEXT") {
                return ElementType::LAYER;
            } else {
                AKLOG_ERROR("Invalid elem type found: {}", elem_str.c_str());
                throw std::runtime_error("");
            }
        };

        core::Rational to_rational(const pybind11::object& pyo) {
            return core::Rational(pyo.attr("numerator").cast<int64_t>(),
                                  pyo.attr("denominator").cast<int64_t>());
        }

    }
}
