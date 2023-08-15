#pragma once

#include <libakcore/path.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <vector>

namespace akashi {
    namespace eval {

        struct KronArg {
            core::Rational play_time;
            long fps; // [TODO] double?
        };

        struct GlobalContext;

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

            core::PlaneContext eval(core::borrowed_ptr<GlobalContext> gctx, const KronArg& arg,
                                    const core::Rational& base_time) const;

            std::vector<core::LayerProfile> computed_profile(const GlobalContext& gctx,
                                                             const core::Rational& base_time) const;

          private:
            core::PlaneContext m_plane_ctx;
        };

        constexpr auto d = sizeof(core::LayerContext);

        class AtomProxy final {
          public:
            explicit AtomProxy(const core::AtomProfile& profile,
                               const std::vector<PlaneProxy>& plane_proxies);

            virtual ~AtomProxy();

            std::vector<core::PlaneContext> eval(core::borrowed_ptr<GlobalContext> gctx,
                                                 const KronArg& arg) const;

            core::AtomProfile computed_profile(const GlobalContext& gctx) const;

            core::AtomStaticProfile static_profile() const;

          private:
            core::AtomProfile m_profile;
            std::vector<PlaneProxy> m_plane_proxies;
        };

        struct GlobalContext {
            std::vector<AtomProxy> atom_proxies;
            std::vector<LayerProxy> layer_proxies;
            core::Rational sec_per_frame;
            core::Rational duration;
            std::string uuid;
        };

    }

}
