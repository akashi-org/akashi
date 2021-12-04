#include "./elem_tracer.h"

#include "./elem.h"
#include "./elem_proxy.h"
#include "./elem_eval.h"
#include "./elem_parser.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <pybind11/embed.h>
namespace py = pybind11;

namespace akashi {
    namespace eval {

        struct AtomTracerContext {
            core::AtomProfile atom_profile;
            std::vector<core::owned_ptr<LayerProxy>> layer_proxies;
            core::Rational atom_duration;
            bool atom_duration_fixed;
        };

        struct LayerTracerContext {
            core::LayerContext layer_ctx = {};
            core::owned_ptr<pybind11::object> params_obj = nullptr;
            core::owned_ptr<pybind11::object> update_func = nullptr;
        };

        void trace_kron_context(const pybind11::object& elem, GlobalContext& ctx) {
            // [TODO]
            // parse atom proxies
            // parse layer proxies
            //

            for (const auto& atom : elem.attr("atoms").cast<py::list>()) {
                AtomTracerContext atom_ctx;
                atom_ctx.atom_profile = {};
                atom_ctx.atom_duration = to_rational(atom.attr("_duration"));

                atom_ctx.atom_profile.uuid = atom.attr("uuid").cast<std::string>();

                for (const auto& layer_idx : atom.attr("layer_indices").cast<py::list>()) {
                    LayerTracerContext layer_trace_ctx = {
                        .layer_ctx = {}, .params_obj = nullptr, .update_func = nullptr};

                    auto layer = elem.attr("layers")[layer_idx];
                    layer_trace_ctx.layer_ctx = parse_layer_context(layer);
                    layer_trace_ctx.params_obj = core::make_owned<pybind11::object>(layer);

                    atom_ctx.layer_proxies.push_back(core::make_owned<LayerProxy>(
                        layer_trace_ctx.layer_ctx, std::move(layer_trace_ctx.params_obj),
                        std::move(layer_trace_ctx.update_func)));
                }

                if (atom_ctx.atom_duration < ctx.interval) {
                    atom_ctx.atom_duration = ctx.interval;
                }
                atom_ctx.atom_profile.from = ctx.duration;
                atom_ctx.atom_profile.to = ctx.duration + atom_ctx.atom_duration;
                atom_ctx.atom_profile.duration = atom_ctx.atom_duration;

                for (auto&& layer_proxy : atom_ctx.layer_proxies) {
                    auto& layer_ctx = layer_proxy->layer_ctx_mut();
                    layer_ctx.atom_uuid = atom_ctx.atom_profile.uuid;
                }

                ctx.atom_proxies.push_back(core::make_owned<AtomProxy>(
                    atom_ctx.atom_profile, std::move(atom_ctx.layer_proxies)));
                ctx.duration += (atom_ctx.atom_duration + ctx.interval);
            }

            // assert(false);
        }

        void trace_root(const pybind11::object& elem, GlobalContext& ctx) {
            auto elem_cl = elem.attr("__closure__");

            for (const auto& handle : elem_cl.cast<py::tuple>()) {
                auto cell_contents = handle.attr("cell_contents");
                // uuid
                if (py::isinstance<py::str>(cell_contents)) {
                    // auto uuid = cell_contents.cast<std::string>();
                    // AKLOG_DEBUG("root: {}", uuid);
                }
                // children
                else if (py::isinstance<py::list>(cell_contents)) {
                    for (const auto& handle2 : cell_contents.cast<py::list>()) {
                        trace_scene(handle2.cast<py::object>(), ctx);
                    }
                }
            }

            if (ctx.atom_proxies.size() > 0) {
                ctx.duration -= ctx.interval;
            }
        }

        void trace_scene(const pybind11::object& elem, GlobalContext& ctx) {
            auto elem_cl = elem.attr("__closure__");

            for (const auto& handle : elem_cl.cast<py::tuple>()) {
                auto cell_contents = handle.attr("cell_contents");
                // uuid
                if (py::isinstance<py::str>(cell_contents)) {
                    // auto uuid = cell_contents.cast<std::string>();
                    // AKLOG_DEBUG("scene: {}", uuid);
                }
                // params
                if (py::isinstance<py::dict>(cell_contents)) {
                }
                // children
                else if (py::isinstance<py::list>(cell_contents)) {
                    for (const auto& handle2 : cell_contents.cast<py::list>()) {
                        trace_atom(handle2.cast<py::object>(), ctx);
                    }
                }
            }
        }

        void trace_atom(const pybind11::object& elem, GlobalContext& ctx) {
            auto elem_cl = elem.attr("__closure__");

            AtomTracerContext atom_ctx;
            atom_ctx.atom_profile = {};
            atom_ctx.atom_duration = core::Rational(0l);
            atom_ctx.atom_duration_fixed = false;
            assert(atom_ctx.layer_proxies.empty());

            for (const auto& handle : elem_cl.cast<py::tuple>()) {
                auto cell_contents = handle.attr("cell_contents");
                // uuid
                if (py::isinstance<py::str>(cell_contents)) {
                    auto uuid = cell_contents.cast<std::string>();
                    atom_ctx.atom_profile.uuid = uuid;
                    // AKLOG_DEBUG("atom: {}", uuid);
                }
                // params
                if (py::isinstance<py::dict>(cell_contents)) {
                    if (cell_contents.cast<py::dict>().contains("duration")) {
                        atom_ctx.atom_duration =
                            to_rational(cell_contents.cast<py::dict>()["duration"]);
                        atom_ctx.atom_duration_fixed = true;
                    }
                }
                // children
                else if (py::isinstance<py::list>(cell_contents)) {
                    for (const auto& handle2 : cell_contents.cast<py::list>()) {
                        trace_layer(handle2.cast<py::object>(), ctx, atom_ctx);
                    }
                }
            }

            // atom proxy
            // min duration
            if (atom_ctx.atom_duration < ctx.interval) {
                atom_ctx.atom_duration = ctx.interval;
            }
            atom_ctx.atom_profile.from = ctx.duration;
            atom_ctx.atom_profile.to = ctx.duration + atom_ctx.atom_duration;
            atom_ctx.atom_profile.duration = atom_ctx.atom_duration;

            for (auto&& layer_proxy : atom_ctx.layer_proxies) {
                auto& layer_ctx = layer_proxy->layer_ctx_mut();
                layer_ctx.atom_uuid = atom_ctx.atom_profile.uuid;
            }

            ctx.atom_proxies.push_back(core::make_owned<AtomProxy>(
                atom_ctx.atom_profile, std::move(atom_ctx.layer_proxies)));
            ctx.duration += (atom_ctx.atom_duration + ctx.interval);
        }

        void trace_layer(const pybind11::object& elem, GlobalContext&,
                         AtomTracerContext& atom_ctx) {
            auto elem_cl = elem.attr("__closure__");

            LayerTracerContext layer_trace_ctx = {
                .layer_ctx = {}, .params_obj = nullptr, .update_func = nullptr};

            for (const auto& handle : elem_cl.cast<py::tuple>()) {
                auto cell_contents = handle.attr("cell_contents");
                // uuid
                if (py::isinstance<py::str>(cell_contents)) {
                    auto uuid = cell_contents.cast<std::string>();
                    layer_trace_ctx.layer_ctx.uuid = uuid;
                    // AKLOG_DEBUG("layer: {}", uuid);
                }
                // layer update
                else if (cell_contents.is_none() || py::hasattr(cell_contents, "__call__")) {
                    layer_trace_ctx.update_func = core::make_owned<pybind11::object>(cell_contents);
                }
                // layer params
                else {
                    layer_trace_ctx.layer_ctx = parse_layer_context(cell_contents);
                    if (auto layer_end = layer_trace_ctx.layer_ctx.to;
                        !atom_ctx.atom_duration_fixed && atom_ctx.atom_duration <= layer_end) {
                        atom_ctx.atom_duration = layer_end;
                    }
                    layer_trace_ctx.params_obj = core::make_owned<pybind11::object>(cell_contents);
                }
            }

            atom_ctx.layer_proxies.push_back(core::make_owned<LayerProxy>(
                layer_trace_ctx.layer_ctx, std::move(layer_trace_ctx.params_obj),
                std::move(layer_trace_ctx.update_func)));
        }

    }
}
