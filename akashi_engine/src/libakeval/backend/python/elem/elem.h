#pragma once

#include <libakcore/rational.h>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        enum class ElementType { ROOT = 0, SCENE, ATOM, LAYER, LENGTH };

        ElementType elem_type(const pybind11::object& elem) noexcept(false);

        core::Rational to_rational(const pybind11::object& pyo);

    }
}
