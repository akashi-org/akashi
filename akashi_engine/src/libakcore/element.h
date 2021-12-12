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

        struct TextLabel {
            std::string color;
            std::string src;
            double radius;
            std::string frag;
            std::string poly;
        };

        struct TextBorder {
            std::string color;
            uint32_t size;
            double radius;
        };

        struct VideoLayerContext {
            std::string src;
            Rational start = core::Rational(0, 1);
            double scale;
            double gain;
            bool stretch = false;
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
            TextLabel label;
            TextAlign text_align;
            TextBorder border;
            std::array<int32_t, 4> pad; // left, right, top, bottom
            int32_t line_span;
            double scale;
            Style style;
            std::string frag;
            std::string poly;
        };

        struct ImageLayerContext {
            std::vector<std::string> srcs;
            bool stretch = false;
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
        };

        struct LayerContext {
            double x;
            double y;
            double z = 0.0;
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

        struct FrameContext {
            Rational pts;
            std::vector<LayerContext> layer_ctxs;
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
            std::vector<LayerProfile> layers;
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
