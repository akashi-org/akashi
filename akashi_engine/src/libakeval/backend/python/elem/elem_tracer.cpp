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
            for (const auto& atom : elem.attr("atoms").cast<py::list>()) {
                AtomTracerContext atom_ctx;
                atom_ctx.atom_profile = {};
                atom_ctx.atom_duration = to_rational(atom.attr("_duration"));

                atom_ctx.atom_profile.uuid = atom.attr("uuid").cast<std::string>();
                atom_ctx.atom_profile.bg_color = atom.attr("bg_color").cast<std::string>();

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

                if (atom_ctx.atom_duration < ctx.sec_per_frame) {
                    atom_ctx.atom_duration = ctx.sec_per_frame;
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
                ctx.duration += atom_ctx.atom_duration;
            }

            // add intervals
            // auto atom_length = ctx.atom_proxies.size();
            // if (atom_length > 1) {
            //     ctx.duration += core::Rational(atom_length - 1, 1) * ctx.interval;
            // }
        }

    }
}
