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
            explicit LayerProxy(const core::LayerContext& layer_ctx);

            virtual ~LayerProxy();

            core::LayerContext eval(const KronArg& arg, const core::Rational& base_time) const;

            const core::LayerContext& layer_ctx() const;

            core::LayerContext& layer_ctx_mut();

            core::LayerProfile computed_profile(const core::Rational& base_time) const;

          private:
            core::LayerContext m_layer_ctx;
        };

        class PlaneProxy final {
          public:
            explicit PlaneProxy(const core::PlaneContext& plane_ctx);

            virtual ~PlaneProxy();

            core::PlaneContext eval(const KronArg& arg, const core::Rational& base_time) const;

            std::vector<core::LayerProfile> computed_profile(const core::Rational& base_time) const;

          private:
            core::PlaneContext m_plane_ctx;
            std::vector<LayerProxy> m_layer_proxies;
        };

        class AtomProxy final {
          public:
            explicit AtomProxy(const core::AtomProfile& profile,
                               const std::vector<PlaneProxy>& plane_proxies);

            virtual ~AtomProxy();

            std::vector<core::PlaneContext> eval(const KronArg& arg) const;

            core::AtomProfile computed_profile() const;

            core::AtomStaticProfile static_profile() const;

          private:
            core::AtomProfile m_profile;
            std::vector<PlaneProxy> m_plane_proxies;
        };

    }
}
