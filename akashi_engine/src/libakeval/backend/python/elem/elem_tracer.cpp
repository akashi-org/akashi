#include "./elem_tracer.h"

#include "./elem.h"
#include "./elem_parser.h"
#include "../../../item.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#define TRACE_TRAIT_FIELD(trait_name)                                                              \
    do {                                                                                           \
        const std::string attr_name = std::string("t_") + std::string(#trait_name) + "s";          \
        const auto& fields_obj = elem.attr(attr_name.c_str()).cast<py::list>();                    \
        ctx.t_##trait_name##s.resize(fields_obj.size());                                           \
        ctx.t_##trait_name##s.shrink_to_fit();                                                     \
        for (size_t i = 0; i < fields_obj.size(); i++) {                                           \
            ctx.t_##trait_name##s[i] =                                                             \
                parse_##trait_name##_tfield(fields_obj[i].cast<py::object>());                     \
        }                                                                                          \
    } while (0);

namespace akashi {
    namespace eval {

        static std::vector<PlaneProxy>
        trace_plane_context(const std::vector<unsigned long>& layer_indices,
                            const GlobalContext& ctx, const size_t level, const size_t atom_idx = 0,
                            const core::AtomStaticProfile* atom_profile = nullptr,
                            const size_t* unit_layer_idx = nullptr) {
            std::vector<PlaneProxy> plane_proxies;

            core::PlaneContext plane_ctx{.level = level};

            if (unit_layer_idx) {
                plane_ctx.base_idx = *unit_layer_idx;
                plane_ctx.base_uuid = ctx.layer_proxies[*unit_layer_idx].layer_ctx().uuid;
            } else {
                plane_ctx.atom_idx = atom_idx;
                plane_ctx.base_uuid = atom_profile->atom_uuid;
            }

            std::vector<unsigned long> unit_layer_indices;
            for (const auto& layer_idx : layer_indices) {
                const auto& layer = ctx.layer_proxies[layer_idx];
                plane_ctx.layer_indices.push_back(layer_idx);
                if (layer.layer_ctx().t_unit) {
                    unit_layer_indices.push_back(layer_idx);
                }
            }

            plane_proxies.push_back(PlaneProxy{plane_ctx});

            for (const auto& unit_layer_idx : unit_layer_indices) {
                const auto& unit_layer = ctx.layer_proxies[unit_layer_idx].layer_ctx();
                assert(unit_layer.t_unit);
                const auto& next_layer_indices = unit_layer.t_unit->layer_indices;
                for (const auto& rest_plane_ctx : trace_plane_context(
                         next_layer_indices, ctx, level + 1, atom_idx, nullptr, &unit_layer_idx)) {
                    plane_proxies.push_back(rest_plane_ctx);
                }
            }

            return plane_proxies;
        }

        void trace_kron_context(const pybind11::object& elem, GlobalContext& ctx) {
            TRACE_TRAIT_FIELD(transform);
            TRACE_TRAIT_FIELD(texture);
            TRACE_TRAIT_FIELD(shader);
            TRACE_TRAIT_FIELD(video);
            TRACE_TRAIT_FIELD(audio);
            TRACE_TRAIT_FIELD(image);
            TRACE_TRAIT_FIELD(text);
            TRACE_TRAIT_FIELD(text_style);
            TRACE_TRAIT_FIELD(rect);
            TRACE_TRAIT_FIELD(circle);
            TRACE_TRAIT_FIELD(tri);
            TRACE_TRAIT_FIELD(line);
            TRACE_TRAIT_FIELD(unit);

            // Layer parsing needs to be done after trait parsing is completely finished
            {
                const auto& layers_obj = elem.attr("layers").cast<py::list>();
                ctx.layer_proxies.resize(layers_obj.size());
                ctx.layer_proxies.shrink_to_fit();
                for (size_t i = 0; i < layers_obj.size(); i++) {
                    const auto& layer_ctx =
                        parse_layer_context(ctx, layers_obj[i].cast<py::object>());
                    ctx.layer_proxies[i] = LayerProxy{layer_ctx};
                }
            }

            // [XXX] After this, the following conditions must be satisfied.
            // 1. The size of ctx.layer_proxies must not be changed
            // 2. The element of ctx.layer_proxies must not be deleted

            for (const auto& atom : elem.attr("atoms").cast<py::list>()) {
                core::AtomStaticProfile atom_profile = {};
                auto atom_duration = to_rational(atom.attr("_duration"));

                atom_profile.uuid = atom.attr("uuid").cast<std::string>();
                atom_profile.atom_uuid = atom_profile.uuid;
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
                    AtomProxy{atom_profile, trace_plane_context(layer_indices, ctx, 0, 0,
                                                                &atom_profile, nullptr)});
                ctx.duration += atom_duration;
            }
        }

    }
}
