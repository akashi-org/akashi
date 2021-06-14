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
            double scale;
            double gain;
            std::vector<std::string> frag;
            std::vector<std::string> geom;
        };

        struct AudioLayerContext {
            std::string src;
            Fraction start;
            double gain;
        };

        struct TextLayerContext {
            std::string text;
            double scale;
            json::optional::type<Style> style;
            std::vector<std::string> frag;
            std::vector<std::string> geom;
        };

        struct ImageLayerContext {
            std::string src;
            double scale;
            std::vector<std::string> frag;
            std::vector<std::string> geom;
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
             * exists iff type == LayerType::AUDIO
             */
            AudioLayerContext audio_layer_ctx;

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
            double gain;
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
