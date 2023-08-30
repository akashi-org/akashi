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
            explicit LayerProxy() = default;

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

        class AtomProxy final {
          public:
            explicit AtomProxy(const core::AtomStaticProfile& profile,
                               const std::vector<PlaneProxy>& plane_proxies);

            virtual ~AtomProxy();

            std::vector<core::PlaneContext> eval(core::borrowed_ptr<GlobalContext> gctx,
                                                 const KronArg& arg) const;

            core::AtomProfile computed_profile(const GlobalContext& gctx) const;

            core::AtomStaticProfile static_profile() const;

          private:
            core::AtomStaticProfile m_profile;
            std::vector<PlaneProxy> m_plane_proxies;
        };

        struct GlobalContext {
            std::vector<AtomProxy> atom_proxies;
            std::vector<LayerProxy> layer_proxies;

            std::vector<core::TransformTField> t_transforms;
            std::vector<core::TextureTField> t_textures;
            std::vector<core::ShaderTField> t_shaders;
            std::vector<core::VideoTField> t_videos;
            std::vector<core::AudioTField> t_audios;
            std::vector<core::ImageTField> t_images;
            std::vector<core::TextTField> t_texts;
            std::vector<core::TextStyleTField> t_text_styles;
            std::vector<core::RectTField> t_rects;
            std::vector<core::CircleTField> t_circles;
            std::vector<core::TriTField> t_tris;
            std::vector<core::LineTField> t_lines;
            std::vector<core::UnitTField> t_units;

            core::Rational sec_per_frame;
            core::Rational duration;
            std::string uuid;

            std::function<core::FrameContext(core::borrowed_ptr<eval::GlobalContext>,
                                             const KronArg&)>
                local_eval;
        };

    }

}
