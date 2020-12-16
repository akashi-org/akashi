#include "./json.h"

#include "./rational.h"
#include "./logger.h"
#include "./element.h"
#include "./common.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace akashi {
    namespace core {

        void from_json(const nlohmann::json& j, Fraction& entry) {
            j.at("num").get_to(entry.num);
            j.at("den").get_to(entry.den);
        }

        void from_json(const nlohmann::json& j, Style& entry) {
            if (j.contains("fontSize")) {
                entry.font_size[json::optional::attr_name] = j["fontSize"].get<uint32_t>();
            }
            if (j.contains("fill")) {
                entry.fill[json::optional::attr_name] = j["fill"].get<std::string>();
            }
            if (j.contains("color")) {
                entry.color[json::optional::attr_name] = j["color"].get<std::string>();
            }
        }

        void from_json(const nlohmann::json& j, LayerContext& entry) {
            j.at("x").get_to(entry.x);
            j.at("y").get_to(entry.y);
            j.at("from").get_to(entry.from);
            j.at("to").get_to(entry.to);
            j.at("__type").get_to(entry.type);
            j.at("__uuid").get_to(entry.uuid);
            j.at("__atom_uuid").get_to(entry.atom_uuid);
            j.at("__display").get_to(entry.display);

            switch (static_cast<LayerType>(entry.type)) {
                case LayerType::VIDEO: {
                    VideoLayerContext layer_ctx;
                    layer_ctx.src = j.at("src");
                    layer_ctx.start = j.at("start");
                    entry.video_layer_ctx = layer_ctx;
                    break;
                }
                case LayerType::TEXT: {
                    TextLayerContext layer_ctx;
                    layer_ctx.text = j.at("text");
                    if (j.contains("style")) {
                        layer_ctx.style[json::optional::attr_name] = j.at("style");
                    }
                    entry.text_layer_ctx = layer_ctx;
                    break;
                }
                case LayerType::IMAGE: {
                    ImageLayerContext layer_ctx;
                    layer_ctx.src = j.at("src");
                    entry.image_layer_ctx = layer_ctx;
                    break;
                }
                default: {
                }
            }
        }

        void from_json(const nlohmann::json& j, FrameContext& entry) {
            j.at("pts").get_to(entry.pts);
            j.at("layer_ctxs").get_to(entry.layer_ctxs);
        }

        void from_json(const nlohmann::json& j, LayerProfile& entry) {
            j.at("type").get_to(entry.type);
            j.at("from").get_to(entry.from);
            j.at("to").get_to(entry.to);
            j.at("uuid").get_to(entry.uuid);
            j.at("src").get_to(entry.src);
            j.at("start").get_to(entry.start);
        }

        void from_json(const nlohmann::json& j, AtomProfile& entry) {
            j.at("from").get_to(entry.from);
            j.at("to").get_to(entry.to);
            j.at("duration").get_to(entry.duration);
            j.at("uuid").get_to(entry.uuid);
            j.at("layers").get_to(entry.layers);
        }

        void from_json(const nlohmann::json& j, RenderProfile& entry) {
            j.at("duration").get_to(entry.duration);
            j.at("uuid").get_to(entry.uuid);
            j.at("atom_profiles").get_to(entry.atom_profiles);
        }

        void from_json(const nlohmann::json& j, RenderContext& entry) {
            j.at("frame_ctxs").get_to(entry.frame_ctxs);
        }

        void parse_render_ctx(nlohmann::json& j, RenderContext& render_ctx) {
            render_ctx = j.get<RenderContext>();
        }

        void parse_render_prof(nlohmann::json& j, RenderProfile& render_prof) {
            render_prof = j.get<RenderProfile>();
        }

    }
}
