#pragma once

#include <libakcore/memory.h>
#include <libakcore/element.h>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        struct KronArg;

        class LayerProxy final {
          public:
            explicit LayerProxy(const core::LayerContext& layer_ctx,
                                core::owned_ptr<pybind11::object>&& params,
                                core::owned_ptr<pybind11::object>&& update);

            virtual ~LayerProxy();

            core::LayerContext eval(const KronArg& arg, const core::Rational& base_time) const;

            const core::LayerContext& layer_ctx() const;

            core::LayerContext& layer_ctx_mut();

            core::LayerProfile computed_profile(const core::Rational& base_time) const;

          private:
            core::LayerContext m_layer_ctx;
            core::owned_ptr<pybind11::object> m_params = nullptr;
            core::owned_ptr<pybind11::object> m_update = nullptr;
        };

        class AtomProxy final {
          public:
            explicit AtomProxy(const core::AtomProfile& profile,
                               std::vector<core::owned_ptr<LayerProxy>>&& layer_proxies);

            virtual ~AtomProxy();

            std::vector<core::LayerContext> eval(const KronArg& arg) const;

            core::AtomProfile computed_profile() const;

            core::AtomStaticProfile static_profile() const;

            const std::vector<core::owned_ptr<LayerProxy>>& layer_proxies() const;

          private:
            core::AtomProfile m_profile;
            std::vector<core::owned_ptr<LayerProxy>> m_layer_proxies;
        };

    }
}
