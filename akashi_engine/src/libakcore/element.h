#pragma once

#include "./rational.h"

#include <string>
#include <vector>
#include <array>

namespace akashi {
    namespace core {

        enum class LayerType { VIDEO = 0, AUDIO, TEXT, IMAGE, EFFECT, SHAPE, LENGTH };

        struct Style {
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

        enum class TextAlign { LEFT = 0, CENTER, RIGHT, LENGTH };

        struct VideoLayerContext {
            std::string src;
            Rational start = core::Rational(0, 1);
            double scale;
            double gain;
            std::string frag;
            std::string poly;
        };

        struct AudioLayerContext {
            std::string src;
            Rational start = core::Rational(0, 1);
            double gain;
        };

        struct TextLayerContext {
            std::string text;
            TextAlign text_align;
            std::array<int32_t, 4> pad; // left, right, top, bottom
            int32_t line_span;
            double scale;
            Style style;
            std::string frag;
            std::string poly;
        };

        struct ImageLayerContext {
            std::vector<std::string> srcs;
            double scale;
            std::string frag;
            std::string poly;
        };

        struct EffectLayerContext {
            std::string frag;
            std::string poly;
        };

        enum class ShapeKind { RECT = 0, CIRCLE, ELLIPSE, TRIANGLE, LINE, LENGTH };

        struct RectDetail {
            int width = 0;
            int height = 0;
        };

        struct CircleDetail {
            double radius;
            int lod;
        };

        struct TriangleDetail {
            double side;
        };

        enum class LineStyle { DEFAULT = 0, ROUND_DOT, SQUARE_DOT, CAP, LENGTH };

        struct LineDetail {
            double size;
            std::array<long, 2> begin;
            std::array<long, 2> end;
            LineStyle style;
        };

        struct ShapeLayerContext {
            ShapeKind shape_kind;
            double border_size;
            double edge_radius;
            bool fill = true;
            std::string color;
            std::string frag;
            std::string poly;
            RectDetail rect;
            CircleDetail circle;
            TriangleDetail tri;
            LineDetail line;
        };

        struct LayerContext {
            double x;
            double y;
            double z = 0.0;
            std::array<long, 2> layer_size = {-1, -1};
            Rational from = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);
            int type;
            std::string uuid;
            std::string atom_uuid;
            bool display = false;

            VideoLayerContext video_layer_ctx;

            AudioLayerContext audio_layer_ctx;

            TextLayerContext text_layer_ctx;

            ImageLayerContext image_layer_ctx;

            EffectLayerContext effect_layer_ctx;

            ShapeLayerContext shape_layer_ctx;
        };

        struct LayerProfile {
            LayerType type;
            Rational from = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);
            std::string uuid;
            std::string src;
            Rational start = core::Rational(0, 1);
            double gain;
        };

        struct AtomProfile {
            Rational from = core::Rational(0, 1);
            Rational to = core::Rational(0, 1);
            Rational duration = core::Rational(0, 1);
            std::string uuid;
            std::string bg_color;
            std::vector<LayerProfile> layers;
        };

        struct AtomStaticProfile {
            std::string bg_color;
        };

        struct FrameContext {
            Rational pts;
            AtomStaticProfile atom_static_profile;
            std::vector<LayerContext> layer_ctxs;
        };

        struct RenderProfile {
            Rational duration = core::Rational(0, 1);
            std::string uuid;
            std::vector<AtomProfile> atom_profiles;
        };

        inline bool has_layers(const RenderProfile& render_prof) {
            for (const auto& atom_prof : render_prof.atom_profiles) {
                if (!atom_prof.layers.empty()) {
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
