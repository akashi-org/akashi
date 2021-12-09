#include "./elem_parser.h"

#include "./elem.h"
#include "../../../item.h"

#include <libakcore/element.h>
#include <libakcore/logger.h>

#include <pybind11/embed.h>

namespace akashi {
    namespace eval {

        static bool has_field(const pybind11::object& entry, const std::string& field_name) {
            const auto& klasses = entry.attr("__class__").attr("mro")().cast<pybind11::list>();
            for (const auto& klass : klasses) {
                if (klass.attr("__name__").cast<std::string>() == field_name) {
                    return true;
                }
            }
            return false;
        }

        static core::Style parse_style(const pybind11::object& style_obj) {
            core::Style style;
            style.font_path = style_obj.attr("font_path").cast<std::string>();
            style.fg_size = style_obj.attr("fg_size").cast<unsigned long>();
            style.fg_color = style_obj.attr("fg_color").cast<std::string>();
            style.use_outline = style_obj.attr("use_outline").cast<bool>();
            style.outline_size = style_obj.attr("outline_size").cast<unsigned long>();
            style.outline_color = style_obj.attr("outline_color").cast<std::string>();
            style.use_shadow = style_obj.attr("use_shadow").cast<bool>();
            style.shadow_size = style_obj.attr("shadow_size").cast<unsigned long>();
            style.shadow_color = style_obj.attr("shadow_color").cast<std::string>();
            return style;
        }

        static std::vector<std::string> parse_shader(const pybind11::object& shader_obj) {
            std::vector<std::string> res_shaders;
            if (!shader_obj.is_none()) {
                res_shaders.push_back(shader_obj.attr("_assemble")().cast<std::string>());
            }
            return res_shaders;
        }

        core::LayerContext parse_layer_context(const pybind11::object& layer_params) {
            core::LayerContext layer_ctx;

            layer_ctx.uuid = layer_params.attr("uuid").cast<std::string>();
            layer_ctx.atom_uuid = layer_params.attr("atom_uuid").cast<std::string>();

            // layer_ctx.display = layer_params.attr("_display").cast<bool>();
            layer_ctx.display = true;

            layer_ctx.from = to_rational(layer_params.attr("atom_offset"));
            layer_ctx.to = (to_rational(layer_params.attr("duration")) +
                            to_rational(layer_params.attr("atom_offset")));

            if (has_field(layer_params, "PositionField")) {
                const auto& pos = layer_params.attr("pos").cast<std::tuple<long, long>>();
                layer_ctx.x = std::get<0>(pos);
                layer_ctx.y = std::get<1>(pos);
                layer_ctx.z = layer_params.attr("z").cast<double>();
            }

            std::string type_str = layer_params.attr("kind").cast<std::string>();

            if (type_str == "VIDEO") {
                layer_ctx.type = static_cast<int>(core::LayerType::VIDEO);
                layer_ctx.video_layer_ctx.src = layer_params.attr("src").cast<std::string>();
                layer_ctx.video_layer_ctx.gain = layer_params.attr("gain").cast<double>();

                layer_ctx.video_layer_ctx.stretch = layer_params.attr("stretch").cast<bool>();

                layer_ctx.video_layer_ctx.start = to_rational(layer_params.attr("start"));
                layer_ctx.video_layer_ctx.scale = 1.0;

                layer_ctx.video_layer_ctx.frag = parse_shader(layer_params.attr("frag_shader"));
                layer_ctx.video_layer_ctx.poly = parse_shader(layer_params.attr("poly_shader"));

            } else if (type_str == "AUDIO") {
                layer_ctx.type = static_cast<int>(core::LayerType::AUDIO);
                layer_ctx.audio_layer_ctx.src = layer_params.attr("src").cast<std::string>();
                layer_ctx.audio_layer_ctx.gain = layer_params.attr("gain").cast<double>();

                // layer_ctx.audio_layer_ctx.start =
                //     to_rational(layer_params.attr("start")).to_fraction();
                layer_ctx.audio_layer_ctx.start = core::Rational(0l);

            } else if (type_str == "IMAGE") {
                layer_ctx.type = static_cast<int>(core::LayerType::IMAGE);
                for (const auto& src : layer_params.attr("srcs").cast<pybind11::list>()) {
                    layer_ctx.image_layer_ctx.srcs.push_back(src.cast<std::string>());
                }

                layer_ctx.image_layer_ctx.stretch = layer_params.attr("stretch").cast<bool>();

                layer_ctx.image_layer_ctx.scale = 1.0;

                layer_ctx.image_layer_ctx.frag = parse_shader(layer_params.attr("frag_shader"));
                layer_ctx.image_layer_ctx.poly = parse_shader(layer_params.attr("poly_shader"));
            } else if (type_str == "TEXT") {
                layer_ctx.type = static_cast<int>(core::LayerType::TEXT);
                layer_ctx.text_layer_ctx.text = layer_params.attr("text").cast<std::string>();
                layer_ctx.text_layer_ctx.style = parse_style(layer_params.attr("style"));
                layer_ctx.text_layer_ctx.scale = 1.0;

                std::string text_align_str = layer_params.attr("text_align").cast<std::string>();
                if (text_align_str == "center") {
                    layer_ctx.text_layer_ctx.text_align = core::TextAlign::CENTER;
                } else if (text_align_str == "right") {
                    layer_ctx.text_layer_ctx.text_align = core::TextAlign::RIGHT;
                } else {
                    layer_ctx.text_layer_ctx.text_align = core::TextAlign::LEFT;
                }

                auto pads = layer_params.attr("pad").cast<std::tuple<long, long, long, long>>();
                layer_ctx.text_layer_ctx.pad[0] = std::get<0>(pads);
                layer_ctx.text_layer_ctx.pad[1] = std::get<1>(pads);
                layer_ctx.text_layer_ctx.pad[2] = std::get<2>(pads);
                layer_ctx.text_layer_ctx.pad[3] = std::get<3>(pads);

                layer_ctx.text_layer_ctx.line_span = layer_params.attr("line_span").cast<int32_t>();

                layer_ctx.text_layer_ctx.frag = parse_shader(layer_params.attr("frag_shader"));
                layer_ctx.text_layer_ctx.poly = parse_shader(layer_params.attr("poly_shader"));
            } else if (type_str == "EFFECT") {
                layer_ctx.type = static_cast<int>(core::LayerType::EFFECT);
                layer_ctx.effect_layer_ctx.frag = parse_shader(layer_params.attr("frag_shader"));
                layer_ctx.effect_layer_ctx.poly = parse_shader(layer_params.attr("poly_shader"));
            } else {
                AKLOG_ERROR("Invalid type '{}' found", type_str.c_str());
                layer_ctx.type = -1;
            }

            return layer_ctx;
        }

    }
}
