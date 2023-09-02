#pragma once

#include "./rational.h"

#include <string>
#include <vector>
#include <array>
#include <map>
#include <functional>
#include <libakcore/memory.h>

namespace akashi {
    namespace eval {
        struct GlobalContext;

    }
    namespace core {

        struct TransformTField {
            double x;
            double y;
            double z = 0.0;
            std::array<long, 2> layer_size = {-1, -1};
            Rational rotation = Rational(0, 1);
            std::array<double, 3> scale = {1.0, 1.0, 1.0}; // (x,y,z)
        };

        struct TextureTField {
            bool uv_flip_v = false;
            bool uv_flip_h = false;
            std::array<long, 2> crop_begin;
            std::array<long, 2> crop_end;
        };

        struct ShaderTField {
            std::string frag;
            std::string poly;
        };

        struct BaseMediaTField {
            std::string src;
            Rational start = core::Rational(0, 1);
            Rational end = core::Rational(0, 1);
            double gain;
        };

        struct VideoTField : public BaseMediaTField {};

        struct AudioTField : public BaseMediaTField {};

        struct ImageTField {
            std::vector<std::string> srcs;
        };

        enum class TextAlign { LEFT = 0, CENTER, RIGHT, LENGTH };

        struct TextTField {
            std::string text;
            TextAlign text_align;
            std::array<int32_t, 4> pad; // left, right, top, bottom
            int32_t line_span;
        };

        struct TextStyleTField {
            std::string font_path;
            uint32_t fg_size;
            std::string fg_color;
            bool use_outline = false;
            uint32_t outline_size;
            std::string outline_color;
            bool use_shadow = false;
            uint32_t shadow_size;
            std::string shadow_color;
        };

        // enum class ShapeKind { RECT = 0, CIRCLE, TRIANGLE, LINE, LENGTH };
        enum class BorderDirection { INNER = 0, OUTER, FULL, LENGTH };

        struct BaseShapeTField {
            std::string fill_color;
            double border_size;
            std::string border_color;
            BorderDirection border_direction = BorderDirection::INNER;
            double edge_radius;
        };

        struct RectTField : public BaseShapeTField {
            int width = 0;
            int height = 0;
        };

        // As the value of the circle size, we are now using TransformTField::layer_size instead.
        struct CircleTField : public BaseShapeTField {};

        struct TriTField : public BaseShapeTField {
            int width = 0;
            int height = 0;
            double wr = 0;
            double hr = 1;
        };

        enum class LineStyle { DEFAULT = 0, ROUND_DOT, SQUARE_DOT, CAP, LENGTH };

        struct LineTField : public BaseShapeTField {
            double size;
            std::array<long, 2> begin;
            std::array<long, 2> end;
            LineStyle style;
        };

        struct UnitTField {
            std::vector<unsigned long> layer_indices;
            std::string bg_color;
            std::array<long, 2> fb_size = {0, 0};
        };

        struct LayerContext {
            std::string uuid;
            std::string atom_uuid;
            std::string key;

            bool display = false;

            Rational from = core::Rational(0, 1);
            Rational layer_local_offset = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);

            core::borrowed_ptr<const TransformTField> t_transform =
                borrowed_ptr<const TransformTField>(nullptr);
            core::borrowed_ptr<const TextureTField> t_texture =
                borrowed_ptr<const TextureTField>(nullptr);
            core::borrowed_ptr<const ShaderTField> t_shader =
                borrowed_ptr<const ShaderTField>(nullptr);

            core::borrowed_ptr<const VideoTField> t_video =
                borrowed_ptr<const VideoTField>(nullptr);
            core::borrowed_ptr<const AudioTField> t_audio =
                borrowed_ptr<const AudioTField>(nullptr);
            core::borrowed_ptr<const ImageTField> t_image =
                borrowed_ptr<const ImageTField>(nullptr);
            core::borrowed_ptr<const TextTField> t_text = borrowed_ptr<const TextTField>(nullptr);
            core::borrowed_ptr<const TextStyleTField> t_text_style =
                borrowed_ptr<const TextStyleTField>(nullptr);

            core::borrowed_ptr<const RectTField> t_rect = borrowed_ptr<const RectTField>(nullptr);
            core::borrowed_ptr<const CircleTField> t_circle =
                borrowed_ptr<const CircleTField>(nullptr);
            core::borrowed_ptr<const TriTField> t_tri = borrowed_ptr<const TriTField>(nullptr);
            core::borrowed_ptr<const LineTField> t_line = borrowed_ptr<const LineTField>(nullptr);

            core::borrowed_ptr<const UnitTField> t_unit = borrowed_ptr<const UnitTField>(nullptr);
        };

        constexpr const int MediaFlagNA = 0;
        constexpr const int MediaFlagVideo = 1 << 0;
        constexpr const int MediaFlagAudio = 1 << 1;
        using MediaType = int;

        inline MediaType get_media_type(const LayerContext& layer_ctx) {
            MediaType type = MediaFlagNA;
            if (layer_ctx.t_video) {
                type |= MediaFlagVideo;
            }
            if (layer_ctx.t_audio) {
                type |= MediaFlagAudio;
            }
            return type;
        };

        struct LayerProfile {
            MediaType type = MediaFlagNA;
            Rational from = core::Rational(0, 1);
            Rational layer_local_offset = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);
            std::string uuid;
            std::string src;
            Rational start = core::Rational(0, 1);
            Rational end = core::Rational(0, 1);
            double gain;
        };

        struct AtomProfile {
            Rational from = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);
            Rational duration = core::Rational(0, 1);
            std::string uuid;
            std::string bg_color;
            std::vector<LayerProfile> av_layers;
        };

        struct AtomStaticProfile {
            Rational from = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);
            Rational duration = core::Rational(0, 1);
            std::string uuid;
            std::string bg_color;
            std::string atom_uuid;
        };

        struct PlaneContext {
            size_t level = 0;
            size_t base_idx; // valid only when level >= 1
            size_t atom_idx; // valid only when level == 0
            std::string base_uuid;
            std::vector<size_t> layer_indices;
            bool display = false;
            std::function<std::vector<core::LayerContext>(core::borrowed_ptr<eval::GlobalContext>,
                                                          const core::PlaneContext&)>
                eval;
            std::function<core::LayerContext(core::borrowed_ptr<eval::GlobalContext>,
                                             const core::PlaneContext&)>
                base;
        };

        struct FrameContext {
            Rational pts;
            AtomStaticProfile atom_static_profile;
            std::vector<PlaneContext> plane_ctxs;
        };

        struct RenderProfile {
            Rational duration = core::Rational(0, 1);
            std::string uuid;
            std::vector<AtomProfile> atom_profiles;
        };

        inline bool has_layers(const RenderProfile& render_prof) {
            for (const auto& atom_prof : render_prof.atom_profiles) {
                if (!atom_prof.av_layers.empty()) {
                    return true;
                }
            }
            return false;
        };

        struct RenderContext {
            std::vector<FrameContext> frame_ctxs;
        };

    }
}
