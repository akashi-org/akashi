#pragma once

#include "./rational.h"

#include <string>
#include <vector>

namespace akashi {
    namespace core {

        enum class LayerType { VIDEO = 0, AUDIO, TEXT, IMAGE, EFFECT, LENGTH };

        struct Style {
            uint32_t font_size;
            std::string font_path;
            std::string fill;
            std::string color;
        };

        struct VideoLayerContext {
            std::string src;
            Fraction start;
            double scale;
            double gain;
            bool stretch = false;
            std::vector<std::string> frag;
            std::vector<std::string> poly;
        };

        struct AudioLayerContext {
            std::string src;
            Fraction start;
            double gain;
        };

        struct TextLayerContext {
            std::string text;
            double scale;
            Style style;
            std::vector<std::string> frag;
            std::vector<std::string> poly;
        };

        struct ImageLayerContext {
            std::vector<std::string> srcs;
            bool stretch = false;
            double scale;
            std::vector<std::string> frag;
            std::vector<std::string> poly;
        };

        struct EffectLayerContext {
            std::vector<std::string> frag;
            std::vector<std::string> poly;
        };

        struct LayerContext {
            double x;
            double y;
            double z = 0.0;
            Fraction from;
            Fraction to;
            int type;
            std::string uuid;
            std::string atom_uuid;
            bool display = false;

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

            /**
             * exists iff type == LayerType::EFFECT
             */
            EffectLayerContext effect_layer_ctx;
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
