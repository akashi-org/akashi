#pragma once

#include "./common.h"
#include "./rational.h"

#include <string>
#include <vector>

namespace akashi {
    namespace core {

        enum class LayerType { VIDEO = 0, AUDIO, TEXT, IMAGE, LENGTH };

        struct Style {
            json::optional::type<uint32_t> font_size;
            json::optional::type<std::string> font_path;
            json::optional::type<std::string> fill;
            json::optional::type<std::string> color;
        };

        struct VideoLayerContext {
            std::string src;
            Fraction start;
            json::optional::type<std::string> frag_path;
            json::optional::type<std::string> geom_path;
        };

        struct TextLayerContext {
            std::string text;
            json::optional::type<Style> style;
            json::optional::type<std::string> frag_path;
            json::optional::type<std::string> geom_path;
        };

        struct ImageLayerContext {
            std::string src;
            json::optional::type<std::string> frag_path;
            json::optional::type<std::string> geom_path;
        };

        struct LayerContext {
            double x;
            double y;
            Fraction from;
            Fraction to;
            int type;
            std::string uuid;
            std::string atom_uuid;
            bool display;

            /**
             * exists iff type == LayerType::VIDEO
             */
            VideoLayerContext video_layer_ctx;

            /**
             * exists iff type == LayerType::TEXT
             */
            TextLayerContext text_layer_ctx;

            /**
             * exists iff type == LayerType::IMAGE
             */
            ImageLayerContext image_layer_ctx;
        };

        struct FrameContext {
            Fraction pts;
            std::vector<LayerContext> layer_ctxs;
        };

        struct LayerProfile {
            LayerType type;
            Fraction from;
            Fraction to;
            std::string uuid;
            std::string src;
            Fraction start;
        };

        struct AtomProfile {
            Fraction from;
            Fraction to;
            Fraction duration;
            std::string uuid;
            std::vector<LayerProfile> layers;
        };

        struct RenderProfile {
            Fraction duration;
            std::string uuid;
            std::vector<AtomProfile> atom_profiles;
        };

        struct RenderContext {
            std::vector<FrameContext> frame_ctxs;
        };

    }
}
