#pragma once

#include <libakcore/element.h>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        core::LayerContext parse_layer_context(const pybind11::object& layer_params);

    }
}
