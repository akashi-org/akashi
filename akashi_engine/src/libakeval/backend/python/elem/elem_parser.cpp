#include "./elem_parser.h"

#include "./elem.h"
#include "../../../item.h"

#include <libakcore/element.h>
#include <libakcore/logger.h>

#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

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

        static std::string parse_shader(const pybind11::object& shader_obj) {
            if (!shader_obj.is_none()) {
                try {
                    return shader_obj.attr("_assemble")().cast<std::string>();
                } catch (const std::exception& e) {
                    AKLOG_ERROR("{}", e.what());
                    return "";
                }
            }
            return "";
        }

        static core::LineStyle parse_line_style(const pybind11::object& style_obj) {
            std::string style_str = style_obj.cast<std::string>();

            if (style_str == "default") {
                return core::LineStyle::DEFAULT;
            } else if (style_str == "round-dot") {
                return core::LineStyle::ROUND_DOT;
            } else if (style_str == "square-dot") {
                return core::LineStyle::SQUARE_DOT;
            } else if (style_str == "cap") {
                return core::LineStyle::CAP;
            } else {
                AKLOG_ERROR("Invalid line style '{}' found", style_str.c_str());
            }

            return core::LineStyle::LENGTH;
        }

        static core::BorderDirection parse_border_direction(const pybind11::object& dir_obj) {
            std::string dir_str = dir_obj.cast<std::string>();

            if (dir_str == "inner") {
                return core::BorderDirection::INNER;
            } else if (dir_str == "outer") {
                return core::BorderDirection::OUTER;
            } else if (dir_str == "full") {
                return core::BorderDirection::FULL;
            } else {
                AKLOG_ERROR("Invalid border direction '{}' found", dir_str.c_str());
            }

            return core::BorderDirection::LENGTH;
        }

        static bool parse_shape_detail(core::LayerContext* layer_ctx,
                                       const pybind11::object& layer_params) {
            std::string kind_str = layer_params.attr("shape_kind").cast<std::string>();
            if (kind_str == "RECT") {
                layer_ctx->shape_layer_ctx.shape_kind = core::ShapeKind::RECT;
                layer_ctx->shape_layer_ctx.rect.width =
                    layer_params.attr("rect").attr("width").cast<long>();
                layer_ctx->shape_layer_ctx.rect.height =
                    layer_params.attr("rect").attr("height").cast<long>();
            } else if (kind_str == "CIRCLE") {
                layer_ctx->shape_layer_ctx.shape_kind = core::ShapeKind::CIRCLE;
            } else if (kind_str == "TRIANGLE") {
                layer_ctx->shape_layer_ctx.shape_kind = core::ShapeKind::TRIANGLE;
                layer_ctx->shape_layer_ctx.tri.width =
                    layer_params.attr("tri").attr("width").cast<long>();
                layer_ctx->shape_layer_ctx.tri.height =
                    layer_params.attr("tri").attr("height").cast<long>();
                layer_ctx->shape_layer_ctx.tri.wr =
                    layer_params.attr("tri").attr("wr").cast<double>();
                layer_ctx->shape_layer_ctx.tri.hr =
                    layer_params.attr("tri").attr("hr").cast<double>();
            } else if (kind_str == "LINE") {
                layer_ctx->shape_layer_ctx.shape_kind = core::ShapeKind::LINE;
                layer_ctx->shape_layer_ctx.line.size =
                    layer_params.attr("line").attr("size").cast<double>();

                const auto& begin =
                    layer_params.attr("line").attr("begin").cast<std::tuple<long, long>>();
                const auto& end =
                    layer_params.attr("line").attr("end").cast<std::tuple<long, long>>();
                layer_ctx->shape_layer_ctx.line.begin = {std::get<0>(begin), std::get<1>(begin)};
                layer_ctx->shape_layer_ctx.line.end = {std::get<0>(end), std::get<1>(end)};

                layer_ctx->shape_layer_ctx.line.style =
                    parse_line_style(layer_params.attr("line").attr("style"));

            } else {
                AKLOG_ERROR("Invalid shape kind '{}' found", kind_str.c_str());
                return false;
            }

            return true;
        }

        static void try_parse_texture_field(core::LayerContext* layer_ctx,
                                            const pybind11::object& layer_params) {
            if (py::hasattr(layer_params, "tex")) {
                layer_ctx->uv_flip_v = layer_params.attr("tex").attr("flip_v").cast<bool>();
                layer_ctx->uv_flip_h = layer_params.attr("tex").attr("flip_h").cast<bool>();
            }
        }

        static void try_parse_transform_field(core::LayerContext* layer_ctx,
                                              const pybind11::object& layer_params) {
            if (py::hasattr(layer_params, "transform")) {
                const auto& pos =
                    layer_params.attr("transform").attr("pos").cast<std::tuple<long, long>>();
                layer_ctx->x = std::get<0>(pos);
                layer_ctx->y = std::get<1>(pos);
                layer_ctx->z = layer_params.attr("transform").attr("z").cast<double>();

                const auto& layer_size = layer_params.attr("transform")
                                             .attr("layer_size")
                                             .cast<std::tuple<long, long>>();
                layer_ctx->layer_size = {std::get<0>(layer_size), std::get<1>(layer_size)};

                layer_ctx->rotation = to_rational(layer_params.attr("transform").attr("rotation"));
            }
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

            try_parse_transform_field(&layer_ctx, layer_params);
            try_parse_texture_field(&layer_ctx, layer_params);

            // media
            // shader

            std::string type_str = layer_params.attr("kind").cast<std::string>();

            if (type_str == "VIDEO") {
                layer_ctx.type = static_cast<int>(core::LayerType::VIDEO);
                layer_ctx.video_layer_ctx.src =
                    layer_params.attr("media").attr("src").cast<std::string>();
                layer_ctx.video_layer_ctx.gain =
                    layer_params.attr("media").attr("gain").cast<double>();
                layer_ctx.video_layer_ctx.start =
                    to_rational(layer_params.attr("media").attr("start"));
                layer_ctx.video_layer_ctx.end = to_rational(layer_params.attr("media").attr("end"));

                layer_ctx.video_layer_ctx.scale = 1.0;

                layer_ctx.video_layer_ctx.frag =
                    parse_shader(layer_params.attr("shader").attr("frag_shader"));
                layer_ctx.video_layer_ctx.poly =
                    parse_shader(layer_params.attr("shader").attr("poly_shader"));

            } else if (type_str == "AUDIO") {
                layer_ctx.type = static_cast<int>(core::LayerType::AUDIO);
                layer_ctx.audio_layer_ctx.src =
                    layer_params.attr("media").attr("src").cast<std::string>();
                layer_ctx.audio_layer_ctx.gain =
                    layer_params.attr("media").attr("gain").cast<double>();
                layer_ctx.audio_layer_ctx.start =
                    to_rational(layer_params.attr("media").attr("start"));
                layer_ctx.audio_layer_ctx.end = to_rational(layer_params.attr("media").attr("end"));

            } else if (type_str == "IMAGE") {
                layer_ctx.type = static_cast<int>(core::LayerType::IMAGE);
                for (const auto& src :
                     layer_params.attr("image").attr("srcs").cast<pybind11::list>()) {
                    layer_ctx.image_layer_ctx.srcs.push_back(src.cast<std::string>());
                }

                layer_ctx.image_layer_ctx.scale = 1.0;

                layer_ctx.image_layer_ctx.frag =
                    parse_shader(layer_params.attr("shader").attr("frag_shader"));
                layer_ctx.image_layer_ctx.poly =
                    parse_shader(layer_params.attr("shader").attr("poly_shader"));

                const auto& crop_begin =
                    layer_params.attr("crop").attr("crop_begin").cast<std::tuple<long, long>>();
                layer_ctx.image_layer_ctx.crop.begin[0] = std::get<0>(crop_begin);
                layer_ctx.image_layer_ctx.crop.begin[1] = std::get<1>(crop_begin);

                const auto& crop_end =
                    layer_params.attr("crop").attr("crop_end").cast<std::tuple<long, long>>();
                layer_ctx.image_layer_ctx.crop.end[0] = std::get<0>(crop_end);
                layer_ctx.image_layer_ctx.crop.end[1] = std::get<1>(crop_end);

            } else if (type_str == "TEXT") {
                layer_ctx.type = static_cast<int>(core::LayerType::TEXT);
                layer_ctx.text_layer_ctx.text =
                    layer_params.attr("text").attr("text").cast<std::string>();
                layer_ctx.text_layer_ctx.style =
                    parse_style(layer_params.attr("text").attr("style"));
                layer_ctx.text_layer_ctx.scale = 1.0;

                std::string text_align_str =
                    layer_params.attr("text").attr("text_align").cast<std::string>();
                if (text_align_str == "center") {
                    layer_ctx.text_layer_ctx.text_align = core::TextAlign::CENTER;
                } else if (text_align_str == "right") {
                    layer_ctx.text_layer_ctx.text_align = core::TextAlign::RIGHT;
                } else {
                    layer_ctx.text_layer_ctx.text_align = core::TextAlign::LEFT;
                }

                auto pads = layer_params.attr("text")
                                .attr("pad")
                                .cast<std::tuple<long, long, long, long>>();
                layer_ctx.text_layer_ctx.pad[0] = std::get<0>(pads);
                layer_ctx.text_layer_ctx.pad[1] = std::get<1>(pads);
                layer_ctx.text_layer_ctx.pad[2] = std::get<2>(pads);
                layer_ctx.text_layer_ctx.pad[3] = std::get<3>(pads);

                layer_ctx.text_layer_ctx.line_span =
                    layer_params.attr("text").attr("line_span").cast<int32_t>();

                layer_ctx.text_layer_ctx.frag =
                    parse_shader(layer_params.attr("shader").attr("frag_shader"));
                layer_ctx.text_layer_ctx.poly =
                    parse_shader(layer_params.attr("shader").attr("poly_shader"));
            } else if (type_str == "UNIT") {
                layer_ctx.type = static_cast<int>(core::LayerType::UNIT);
                layer_ctx.unit_layer_ctx.layer_indices = layer_params.attr("unit")
                                                             .attr("layer_indices")
                                                             .cast<std::vector<unsigned long>>();
                layer_ctx.unit_layer_ctx.bg_color =
                    layer_params.attr("unit").attr("bg_color").cast<std::string>();
                const auto& fb_size =
                    layer_params.attr("unit").attr("fb_size").cast<std::tuple<long, long>>();
                layer_ctx.unit_layer_ctx.fb_size = {std::get<0>(fb_size), std::get<1>(fb_size)};
                layer_ctx.unit_layer_ctx.frag =
                    parse_shader(layer_params.attr("shader").attr("frag_shader"));
                layer_ctx.unit_layer_ctx.poly =
                    parse_shader(layer_params.attr("shader").attr("poly_shader"));
            } else if (type_str == "SHAPE") {
                layer_ctx.type = static_cast<int>(core::LayerType::SHAPE);
                layer_ctx.shape_layer_ctx.frag =
                    parse_shader(layer_params.attr("shader").attr("frag_shader"));
                layer_ctx.shape_layer_ctx.poly =
                    parse_shader(layer_params.attr("shader").attr("poly_shader"));

                layer_ctx.shape_layer_ctx.border_size =
                    layer_params.attr("shape").attr("border_size").cast<double>();
                layer_ctx.shape_layer_ctx.border_color =
                    layer_params.attr("shape").attr("border_color").cast<std::string>();

                layer_ctx.shape_layer_ctx.border_direction =
                    parse_border_direction(layer_params.attr("shape").attr("border_direction"));

                layer_ctx.shape_layer_ctx.edge_radius =
                    layer_params.attr("shape").attr("edge_radius").cast<double>();
                layer_ctx.shape_layer_ctx.fill =
                    layer_params.attr("shape").attr("fill").cast<bool>();
                layer_ctx.shape_layer_ctx.color =
                    layer_params.attr("shape").attr("color").cast<std::string>();

                parse_shape_detail(&layer_ctx, layer_params.attr("shape"));
            } else {
                AKLOG_ERROR("Invalid type '{}' found", type_str.c_str());
                layer_ctx.type = -1;
            }

            return layer_ctx;
        }

    }
}
