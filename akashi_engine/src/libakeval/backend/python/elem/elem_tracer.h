#pragma once

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        struct GlobalContext;
        struct AtomTracerContext;

        void trace_kron_context(const pybind11::object& elem, GlobalContext& ctx);

    }
}
