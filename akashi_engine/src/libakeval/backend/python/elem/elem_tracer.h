#pragma once

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        struct GlobalContext;
        struct AtomTracerContext;

        void trace_kron_context(const pybind11::object& elem, GlobalContext& ctx);

        void trace_root(const pybind11::object& elem, GlobalContext& ctx);

        void trace_scene(const pybind11::object& elem, GlobalContext& ctx);

        void trace_atom(const pybind11::object& elem, GlobalContext& ctx);

        void trace_layer(const pybind11::object& elem, GlobalContext& ctx,
                         AtomTracerContext& atom_ctx);

    }
}
