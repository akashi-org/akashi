#pragma once

#include <libakcore/rational.h>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        core::Rational to_rational(const pybind11::object& pyo);

    }
}
