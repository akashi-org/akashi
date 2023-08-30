#include "./item.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        LayerProxy::LayerProxy(const core::LayerContext& layer_ctx) : m_layer_ctx(layer_ctx){};

        LayerProxy::~LayerProxy() {
            m_layer_ctx.t_transform = borrowed_ptr<const TransformTField>(nullptr);
            m_layer_ctx.t_texture = borrowed_ptr<const TextureTField>(nullptr);
            m_layer_ctx.t_shader = borrowed_ptr<const ShaderTField>(nullptr);
            m_layer_ctx.t_video = borrowed_ptr<const VideoTField>(nullptr);
            m_layer_ctx.t_audio = borrowed_ptr<const AudioTField>(nullptr);
            m_layer_ctx.t_image = borrowed_ptr<const ImageTField>(nullptr);
            m_layer_ctx.t_text = borrowed_ptr<const TextTField>(nullptr);
            m_layer_ctx.t_text_style = borrowed_ptr<const TextStyleTField>(nullptr);
            m_layer_ctx.t_rect = borrowed_ptr<const RectTField>(nullptr);
            m_layer_ctx.t_circle = borrowed_ptr<const CircleTField>(nullptr);
            m_layer_ctx.t_tri = borrowed_ptr<const TriTField>(nullptr);
            m_layer_ctx.t_line = borrowed_ptr<const LineTField>(nullptr);
            m_layer_ctx.t_unit = borrowed_ptr<const UnitTField>(nullptr);
        };

        core::LayerContext LayerProxy::eval(const KronArg& arg,
                                            const core::Rational& base_time) const {
            core::LayerContext layer_ctx = m_layer_ctx;

            layer_ctx.from = m_layer_ctx.from + base_time;
            layer_ctx.to = m_layer_ctx.to + base_time;

            layer_ctx.display = (layer_ctx.from <= arg.play_time) && (arg.play_time < layer_ctx.to);
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
            computed.type = core::get_media_type(m_layer_ctx);

            core::BaseMediaTField media_field;
            if (computed.type & core::MediaFlagVideo) {
                media_field = *m_layer_ctx.t_video;
            } else if (computed.type & core::MediaFlagAudio) {
                media_field = *m_layer_ctx.t_audio;
            }
            if (computed.type > 0) {
                computed.src = media_field.src;
                computed.start = media_field.start;
                computed.end = media_field.end;
                computed.gain = media_field.gain;
            }
            return computed;
        }

        PlaneProxy::PlaneProxy(const core::PlaneContext& plane_ctx) : m_plane_ctx(plane_ctx){};

        PlaneProxy::~PlaneProxy(){};

        core::PlaneContext PlaneProxy::eval(core::borrowed_ptr<GlobalContext> gctx,
                                            const KronArg& arg,
                                            const core::Rational& base_time) const {
            core::PlaneContext plane_ctx;

            core::Rational plane_from;
            core::Rational plane_to;
            if (m_plane_ctx.level == 0) {
                const auto& atom_proxy = gctx->atom_proxies[m_plane_ctx.atom_idx];
                plane_from = atom_proxy.static_profile().from + base_time;
                plane_to = atom_proxy.static_profile().to + base_time;
            } else {
                const auto& base_layer_proxy = gctx->layer_proxies[m_plane_ctx.base_idx];
                plane_from = base_layer_proxy.layer_ctx().from + base_time;
                plane_to = base_layer_proxy.layer_ctx().to + base_time;
            }

            plane_ctx.display = (plane_from <= arg.play_time) && (arg.play_time < plane_to);
            if (!plane_ctx.display) {
                return plane_ctx;
            }
            plane_ctx = m_plane_ctx;
            plane_ctx.display = true;

            plane_ctx.eval = [arg, base_time](core::borrowed_ptr<eval::GlobalContext> gctx,
                                              const core::PlaneContext& inner_plane_ctx) {
                std::vector<core::LayerContext> layer_ctxs;
                if (!gctx) {
                    AKLOG_ERRORN("gctx is null");
                    return layer_ctxs;
                }
                if (inner_plane_ctx.display) {
                    for (const auto& layer_idx : inner_plane_ctx.layer_indices) {
                        const auto& layer_proxy = gctx->layer_proxies[layer_idx];
                        layer_ctxs.push_back(layer_proxy.eval(arg, base_time));
                    }
                }
                return layer_ctxs;
            };

            plane_ctx.base = [](core::borrowed_ptr<eval::GlobalContext> gctx,
                                const core::PlaneContext& inner_plane_ctx) {
                core::LayerContext base;
                if (!gctx) {
                    AKLOG_ERRORN("gctx is null");
                    return base;
                }
                if (inner_plane_ctx.level == 0) {
                    AKLOG_ERRORN("plane level is zero");
                    return base;
                }
                return gctx->layer_proxies[inner_plane_ctx.base_idx].layer_ctx();
            };

            return plane_ctx;
        };

        std::vector<core::LayerProfile>
        PlaneProxy::computed_profile(const GlobalContext& gctx,
                                     const core::Rational& base_time) const {
            std::vector<core::LayerProfile> avlayer_profiles;
            for (const auto& layer_idx : m_plane_ctx.layer_indices) {
                const auto& layer_proxy = gctx.layer_proxies[layer_idx];
                auto layer_type = core::get_media_type(layer_proxy.layer_ctx());
                if (layer_type > 0) {
                    avlayer_profiles.push_back(layer_proxy.computed_profile(base_time));
                }
            }
            return avlayer_profiles;
        }

        AtomProxy::AtomProxy(const core::AtomStaticProfile& profile,
                             const std::vector<PlaneProxy>& plane_proxies)
            : m_profile(profile), m_plane_proxies(plane_proxies){};

        AtomProxy::~AtomProxy(){};

        std::vector<core::PlaneContext> AtomProxy::eval(core::borrowed_ptr<GlobalContext> gctx,
                                                        const KronArg& arg) const {
            std::vector<core::PlaneContext> plane_ctxs;
            for (const auto& plane_proxy : m_plane_proxies) {
                const auto& r_plane_ctx = plane_proxy.eval(gctx, arg, m_profile.from);
                if (r_plane_ctx.display) {
                    plane_ctxs.push_back(r_plane_ctx);
                }
            }
            return plane_ctxs;
        };

        core::AtomProfile AtomProxy::computed_profile(const GlobalContext& gctx) const {
            core::AtomProfile computed;
            computed.from = m_profile.from;
            computed.to = m_profile.to;
            computed.duration = m_profile.duration;
            computed.uuid = m_profile.uuid;
            computed.bg_color = m_profile.bg_color;
            computed.av_layers = {};
            for (const auto& plane_proxy : m_plane_proxies) {
                for (const auto& layer_profile :
                     plane_proxy.computed_profile(gctx, m_profile.from)) {
                    computed.av_layers.push_back(layer_profile);
                }
            }
            return computed;
        }

        core::AtomStaticProfile AtomProxy::static_profile() const { return m_profile; }

    }
}
