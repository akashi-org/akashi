#include "./elem_proxy.h"

#include "./elem_parser.h"
#include "../../../item.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>

#include <pybind11/embed.h>
namespace py = pybind11;

namespace akashi {
    namespace eval {

        namespace detail {

            py::object Second(const core::Rational& rat) {
                auto callable = py::module_::import("akashi_core").attr("time").attr("Second");
                return callable(rat.num(), rat.den());
            }

            py::object KronArgs(const core::Rational& playtime, const long fps) {
                auto callable = py::module_::import("akashi_core").attr("kron").attr("KronArgs");
                return callable(detail::Second(playtime), fps);
            }

            py::object layer_update(const py::object& func, const KronArg& arg,
                                    const py::object& params_obj,
                                    const core::LayerContext& params) {
                auto kron_args_obj = KronArgs(arg.play_time, arg.fps);

                auto comp_params_obj =
                    params_obj.attr("_update")(detail::Second(to_rational(params.from)),
                                               detail::Second(to_rational(params.to)));
                comp_params_obj.attr("_uuid") = py::str(params.uuid);
                comp_params_obj.attr("_atom_uuid") = py::str(params.atom_uuid);
                comp_params_obj.attr("_display") = py::bool_(params.display);

                return func(kron_args_obj, comp_params_obj);
            }

        }

        LayerProxy::LayerProxy(const core::LayerContext& layer_ctx,
                               core::owned_ptr<pybind11::object>&& params,
                               core::owned_ptr<pybind11::object>&& update)
            : m_layer_ctx(layer_ctx), m_params(std::move(params)), m_update(std::move(update)){};

        LayerProxy::~LayerProxy(){};

        core::LayerContext LayerProxy::eval(const KronArg& arg,
                                            const core::Rational& base_time) const {
            core::LayerContext layer_ctx = m_layer_ctx;
            layer_ctx.display = false;

            layer_ctx.from = (to_rational(m_layer_ctx.from) + base_time).to_fraction();
            layer_ctx.to = (to_rational(m_layer_ctx.to) + base_time).to_fraction();

            if (to_rational(layer_ctx.from) <= arg.play_time &&
                arg.play_time <= to_rational(layer_ctx.to)) {
                layer_ctx.display = true;
                if (m_update && !m_update->is_none()) {
                    return parse_layer_context(
                        detail::layer_update(*m_update, arg, *m_params, layer_ctx));
                } else {
                    return layer_ctx;
                }
            } else {
                return layer_ctx;
            }
        };

        const core::LayerContext& LayerProxy::layer_ctx() const { return m_layer_ctx; }

        core::LayerContext& LayerProxy::layer_ctx_mut() { return m_layer_ctx; }

        core::LayerProfile LayerProxy::computed_profile(const core::Rational& base_time) const {
            core::LayerProfile computed{};
            computed.uuid = m_layer_ctx.uuid;
            computed.from = (to_rational(m_layer_ctx.from) + base_time).to_fraction();
            computed.to = (to_rational(m_layer_ctx.to) + base_time).to_fraction();
            computed.type = static_cast<core::LayerType>(m_layer_ctx.type);
            switch (computed.type) {
                case core::LayerType::VIDEO: {
                    computed.src = m_layer_ctx.video_layer_ctx.src;
                    computed.start = m_layer_ctx.video_layer_ctx.start;
                    computed.gain = m_layer_ctx.video_layer_ctx.gain;
                    break;
                }
                case core::LayerType::AUDIO: {
                    computed.src = m_layer_ctx.audio_layer_ctx.src;
                    computed.start = m_layer_ctx.audio_layer_ctx.start;
                    computed.gain = m_layer_ctx.audio_layer_ctx.gain;
                    break;
                }
                default: {
                    break;
                }
            }
            return computed;
        }

        AtomProxy::AtomProxy(const core::AtomProfile& profile,
                             std::vector<core::owned_ptr<LayerProxy>>&& layer_proxies)
            : m_profile(profile), m_layer_proxies(std::move(layer_proxies)){};

        AtomProxy::~AtomProxy(){};

        std::vector<core::LayerContext> AtomProxy::eval(const KronArg& arg) const {
            std::vector<core::LayerContext> layer_ctxs;
            for (const auto& layer_proxy : m_layer_proxies) {
                auto layer_ctx = layer_proxy->eval(arg, to_rational(m_profile.from));
                layer_ctxs.push_back(layer_ctx);
            }
            return layer_ctxs;
        };

        core::AtomProfile AtomProxy::computed_profile() const {
            core::AtomProfile computed = m_profile;
            computed.layers = {};
            for (const auto& layer_proxy : m_layer_proxies) {
                auto layer_type = static_cast<core::LayerType>(layer_proxy->layer_ctx().type);
                if (layer_type == core::LayerType::VIDEO || layer_type == core::LayerType::AUDIO) {
                    computed.layers.push_back(
                        layer_proxy->computed_profile(to_rational(m_profile.from)));
                }
            }
            return computed;
        }

        const std::vector<core::owned_ptr<LayerProxy>>& AtomProxy::layer_proxies() const {
            return m_layer_proxies;
        }

    }
}
