#include "./elem_tracer.h"

#include "./elem.h"
#include "./elem_parser.h"
#include "../../../item.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

namespace akashi {
    namespace eval {

        static std::vector<PlaneProxy>
        trace_plane_context(const std::vector<unsigned long>& layer_indices,
                            const GlobalContext& ctx, const size_t level,
                            const size_t* unit_layer_idx = nullptr) {
            std::vector<PlaneProxy> plane_proxies;

            core::PlaneContext plane_ctx{.level = level};
            if (unit_layer_idx) {
                plane_ctx.base_idx = *unit_layer_idx;
            }

            std::vector<unsigned long> unit_layer_indices;
            for (const auto& layer_idx : layer_indices) {
                const auto& layer = ctx.layer_proxies[layer_idx];

                plane_ctx.layer_indices.push_back(layer_idx);

                if (static_cast<core::LayerType>(layer.layer_ctx().type) == core::LayerType::UNIT) {
                    unit_layer_indices.push_back(layer_idx);
                }
            }

            plane_proxies.push_back(PlaneProxy{plane_ctx});

            for (const auto& unit_layer_idx : unit_layer_indices) {
                const auto& unit_layer_ctx =
                    ctx.layer_proxies[unit_layer_idx].layer_ctx().unit_layer_ctx;
                for (const auto& rest_plane_ctx : trace_plane_context(
                         unit_layer_ctx.layer_indices, ctx, level + 1, &unit_layer_idx)) {
                    plane_proxies.push_back(rest_plane_ctx);
                }
            }

            return plane_proxies;
        }

        void trace_kron_context(const pybind11::object& elem, GlobalContext& ctx) {
            for (const auto& layer_obj : elem.attr("layers").cast<py::list>()) {
                auto layer_ctx = parse_layer_context(layer_obj.cast<py::object>());
                ctx.layer_proxies.push_back(LayerProxy{layer_ctx});
            }

            for (const auto& atom : elem.attr("atoms").cast<py::list>()) {
                core::AtomProfile atom_profile = {};
                auto atom_duration = to_rational(atom.attr("_duration"));

                atom_profile.uuid = atom.attr("uuid").cast<std::string>();
                atom_profile.bg_color = atom.attr("bg_color").cast<std::string>();

                if (atom_duration < ctx.sec_per_frame) {
                    atom_duration = ctx.sec_per_frame;
                }
                atom_profile.from = ctx.duration;
                atom_profile.to = ctx.duration + atom_duration;
                atom_profile.duration = atom_duration;

                auto layer_indices = atom.attr("layer_indices").cast<std::vector<unsigned long>>();
                for (const auto& layer_idx : layer_indices) {
                    auto& layer_ctx = ctx.layer_proxies[layer_idx].layer_ctx_mut();
                    layer_ctx.atom_uuid = atom_profile.uuid;
                }

                ctx.atom_proxies.push_back(
                    AtomProxy{atom_profile, trace_plane_context(layer_indices, ctx, 0, nullptr)});
                ctx.duration += atom_duration;
            }
        }

    }
}
