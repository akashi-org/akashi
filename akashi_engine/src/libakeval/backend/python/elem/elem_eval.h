#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <vector>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        struct KronArg;
        struct GlobalContext;

        core::owned_ptr<GlobalContext> global_eval(const pybind11::object& elem,
                                                   const core::Rational& fps);

    }
}
