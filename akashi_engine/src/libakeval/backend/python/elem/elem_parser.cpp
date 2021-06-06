#include "./elem_parser.h"

#include "./elem.h"
#include "../../../item.h"

#include <libakcore/element.h>
#include <libakcore/logger.h>
#include <libakcore/common.h>

#include <pybind11/embed.h>

namespace akashi {
    namespace eval {

        static core::Style parse_style(const pybind11::dict& dict_obj) {
            core::Style style;

            if (dict_obj.contains("font_size")) {
                style.font_size[core::json::optional::attr_name] =
                    dict_obj["font_size"].cast<unsigned long>();
            }
            if (dict_obj.contains("font_path")) {
                style.font_path[core::json::optional::attr_name] =
                    dict_obj["font_path"].cast<std::string>();
            }
            if (dict_obj.contains("fill")) {
                style.fill[core::json::optional::attr_name] = dict_obj["fill"].cast<std::string>();
            }
            if (dict_obj.contains("color")) {
                style.color[core::json::optional::attr_name] =
                    dict_obj["color"].cast<std::string>();
            }

            return style;
        }

        core::LayerContext parse_layer_context(const pybind11::object& layer_params) {
            core::LayerContext layer_ctx;

            layer_ctx.from = to_rational(layer_params.attr("begin")).to_fraction();
            layer_ctx.to = to_rational(layer_params.attr("end")).to_fraction();
            layer_ctx.uuid = layer_params.attr("_uuid").cast<std::string>();
            layer_ctx.atom_uuid = layer_params.attr("_atom_uuid").cast<std::string>();
            layer_ctx.x = layer_params.attr("x").cast<long>();
            layer_ctx.y = layer_params.attr("y").cast<long>();
            layer_ctx.display = layer_params.attr("_display").cast<bool>();

            std::string type_str = layer_params.attr("_type").cast<std::string>();

            if (type_str == "VIDEO") {
                layer_ctx.type = static_cast<int>(core::LayerType::VIDEO);
                layer_ctx.video_layer_ctx.src = layer_params.attr("src").cast<std::string>();
                layer_ctx.video_layer_ctx.start =
                    to_rational(layer_params.attr("start")).to_fraction();
                layer_ctx.video_layer_ctx.scale = layer_params.attr("scale").cast<double>();
                layer_ctx.video_layer_ctx.gain = layer_params.attr("gain").cast<double>();
                layer_ctx.video_layer_ctx.frag_path[core::json::optional::attr_name] =
                    layer_params.attr("frag_path").cast<std::string>();
                layer_ctx.video_layer_ctx.geom_path[core::json::optional::attr_name] =
                    layer_params.attr("geom_path").cast<std::string>();
            } else if (type_str == "AUDIO") {
                layer_ctx.type = static_cast<int>(core::LayerType::AUDIO);
            } else if (type_str == "IMAGE") {
                layer_ctx.type = static_cast<int>(core::LayerType::IMAGE);
                layer_ctx.image_layer_ctx.src = layer_params.attr("src").cast<std::string>();
                layer_ctx.image_layer_ctx.scale = layer_params.attr("scale").cast<double>();
                layer_ctx.image_layer_ctx.frag_path[core::json::optional::attr_name] =
                    layer_params.attr("frag_path").cast<std::string>();
                layer_ctx.image_layer_ctx.geom_path[core::json::optional::attr_name] =
                    layer_params.attr("geom_path").cast<std::string>();
            } else if (type_str == "TEXT") {
                layer_ctx.type = static_cast<int>(core::LayerType::TEXT);
                layer_ctx.text_layer_ctx.text = layer_params.attr("text").cast<std::string>();
                layer_ctx.text_layer_ctx.style[core::json::optional::attr_name] =
                    parse_style(layer_params.attr("style"));
                layer_ctx.text_layer_ctx.scale = layer_params.attr("scale").cast<double>();
                layer_ctx.text_layer_ctx.frag_path[core::json::optional::attr_name] =
                    layer_params.attr("frag_path").cast<std::string>();
                layer_ctx.text_layer_ctx.geom_path[core::json::optional::attr_name] =
                    layer_params.attr("geom_path").cast<std::string>();
            } else {
                AKLOG_ERROR("Invalid type '{}' found", type_str.c_str());
                layer_ctx.type = -1;
            }

            return layer_ctx;
        }

    }
}
