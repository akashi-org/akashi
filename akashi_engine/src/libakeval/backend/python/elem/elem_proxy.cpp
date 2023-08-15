#include "./elem_proxy.h"

#include "./elem_eval.h"
#include "../../../item.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>

namespace akashi {
    namespace eval {

        LayerProxy::LayerProxy(const core::LayerContext& layer_ctx) : m_layer_ctx(layer_ctx){};

        LayerProxy::~LayerProxy(){};

        core::LayerContext LayerProxy::eval(const KronArg& arg,
                                            const core::Rational& base_time) const {
            core::LayerContext layer_ctx = m_layer_ctx;

            layer_ctx.from = m_layer_ctx.from + base_time;
            layer_ctx.to = m_layer_ctx.to + base_time;

            layer_ctx.display =
                (layer_ctx.from <= arg.play_time) && (arg.play_time <= layer_ctx.to);
            return layer_ctx;
        };

        const core::LayerContext& LayerProxy::layer_ctx() const { return m_layer_ctx; }

        core::LayerContext& LayerProxy::layer_ctx_mut() { return m_layer_ctx; }

        core::LayerProfile LayerProxy::computed_profile(const core::Rational& base_time) const {
            core::LayerProfile computed{};
            computed.uuid = m_layer_ctx.uuid;
            computed.from = m_layer_ctx.from + base_time;
            computed.layer_local_offset = m_layer_ctx.layer_local_offset;
            computed.to = m_layer_ctx.to + base_time;
            computed.type = static_cast<core::LayerType>(m_layer_ctx.type);
            switch (computed.type) {
                case core::LayerType::VIDEO: {
                    computed.src = m_layer_ctx.video_layer_ctx.src;
                    computed.start = m_layer_ctx.video_layer_ctx.start;
                    computed.end = m_layer_ctx.video_layer_ctx.end;
                    computed.gain = m_layer_ctx.video_layer_ctx.gain;
                    break;
                }
                case core::LayerType::AUDIO: {
                    computed.src = m_layer_ctx.audio_layer_ctx.src;
                    computed.start = m_layer_ctx.audio_layer_ctx.start;
                    computed.end = m_layer_ctx.audio_layer_ctx.end;
                    computed.gain = m_layer_ctx.audio_layer_ctx.gain;
                    break;
                }
                default: {
                    break;
                }
            }
            return computed;
        }

        PlaneProxy::PlaneProxy(const core::PlaneContext& plane_ctx) : m_plane_ctx(plane_ctx){};

        PlaneProxy::~PlaneProxy(){};

        core::PlaneContext PlaneProxy::eval(const GlobalContext& gctx, const KronArg& arg,
                                            const core::Rational& base_time) const {
            core::PlaneContext plane_ctx;
            plane_ctx.level = m_plane_ctx.level;
            plane_ctx.base_idx = m_plane_ctx.base_idx;

            // [TODO] bugs might occur in a case where base is atom
            const auto& layer_proxy = gctx.layer_proxies[plane_ctx.base_idx];

            plane_ctx.from = layer_proxy.layer_ctx().from + base_time;
            plane_ctx.to = layer_proxy.layer_ctx().to + base_time;
            plane_ctx.display =
                (plane_ctx.from <= arg.play_time) && (arg.play_time <= plane_ctx.to);

            // if (plane_ctx.base_idx.display) {
            //     for (const auto& layer_proxy : m_layer_proxies) {
            //         plane_ctx.layer_indices.push_back(layer_proxy.eval(arg, base_time));
            //     }
            // }

            return plane_ctx;
        };

        std::vector<core::LayerProfile>
        PlaneProxy::computed_profile(const GlobalContext& gctx,
                                     const core::Rational& base_time) const {
            std::vector<core::LayerProfile> avlayer_profiles;
            for (const auto& layer_idx : m_plane_ctx.layer_indices) {
                const auto& layer_proxy = gctx.layer_proxies[layer_idx];
                auto layer_type = static_cast<core::LayerType>(layer_proxy.layer_ctx().type);
                if (layer_type == core::LayerType::VIDEO || layer_type == core::LayerType::AUDIO) {
                    avlayer_profiles.push_back(layer_proxy.computed_profile(base_time));
                }
            }
            return avlayer_profiles;
        }

        AtomProxy::AtomProxy(const core::AtomProfile& profile,
                             const std::vector<PlaneProxy>& plane_proxies)
            : m_profile(profile), m_plane_proxies(plane_proxies){};

        AtomProxy::~AtomProxy(){};

        std::vector<core::PlaneContext> AtomProxy::eval(const GlobalContext& gctx,
                                                        const KronArg& arg) const {
            std::vector<core::PlaneContext> plane_ctxs;
            for (const auto& plane_proxy : m_plane_proxies) {
                plane_ctxs.push_back(plane_proxy.eval(gctx, arg, m_profile.from));
            }
            return plane_ctxs;
        };

        core::AtomProfile AtomProxy::computed_profile(const GlobalContext& gctx) const {
            core::AtomProfile computed = m_profile;
            computed.av_layers = {};
            for (const auto& plane_proxy : m_plane_proxies) {
                for (const auto& layer_profile :
                     plane_proxy.computed_profile(gctx, m_profile.from)) {
                    computed.av_layers.push_back(layer_profile);
                }
            }
            return computed;
        }

        core::AtomStaticProfile AtomProxy::static_profile() const {
            core::AtomStaticProfile static_profile;
            static_profile.bg_color = m_profile.bg_color;
            static_profile.atom_uuid = m_profile.uuid;
            return static_profile;
        }

    }
}
