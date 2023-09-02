#include "./elem.h"

#include <libakcore/logger.h>

#include <pybind11/embed.h>
#include <string>
#include <stdexcept>

namespace akashi {
    namespace eval {

        core::Rational to_rational(const pybind11::object& pyo) {
            return core::Rational(pyo.attr("numerator").cast<int64_t>(),
                                  pyo.attr("denominator").cast<int64_t>());
        }

    }
}
