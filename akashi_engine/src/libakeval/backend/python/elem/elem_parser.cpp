#include "./elem_parser.h"

#include "./elem.h"
#include "../../../item.h"

#include <libakcore/element.h>
#include <libakcore/logger.h>

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#define PARSE_TFIELD_INDEX(field_name)                                                             \
    do {                                                                                           \
        if (auto field_idx = entry.attr(#field_name).cast<long long>(); field_idx > -1) {          \
            layer_ctx.field_name = core::borrowed_ptr(&ctx.field_name##s[field_idx]);              \
        }                                                                                          \
    } while (0)

namespace akashi {
    namespace eval {

        core::LayerContext parse_layer_context(const GlobalContext& ctx,
                                               const pybind11::object& entry) {
            core::LayerContext layer_ctx;

            layer_ctx.uuid = entry.attr("uuid").cast<std::string>();
            layer_ctx.atom_uuid = entry.attr("atom_uuid").cast<std::string>();
            layer_ctx.key = entry.attr("key").cast<std::string>();

            // layer_ctx.display = layer_params.attr("_display").cast<bool>();
            layer_ctx.display = true;

            layer_ctx.from = to_rational(entry.attr("slice_offset"));
            layer_ctx.layer_local_offset = to_rational(entry.attr("layer_local_offset"));
            layer_ctx.to =
                (to_rational(entry.attr("_duration")) + to_rational(entry.attr("slice_offset")));

            PARSE_TFIELD_INDEX(t_transform);
            PARSE_TFIELD_INDEX(t_texture);
            PARSE_TFIELD_INDEX(t_shader);
            PARSE_TFIELD_INDEX(t_video);
            PARSE_TFIELD_INDEX(t_audio);
            PARSE_TFIELD_INDEX(t_image);
            PARSE_TFIELD_INDEX(t_text);
            PARSE_TFIELD_INDEX(t_text_style);
            PARSE_TFIELD_INDEX(t_rect);
            PARSE_TFIELD_INDEX(t_circle);
            PARSE_TFIELD_INDEX(t_tri);
            PARSE_TFIELD_INDEX(t_line);
            PARSE_TFIELD_INDEX(t_unit);

            return layer_ctx;
        }

        core::TransformTField parse_transform_tfield(const pybind11::object& entry) {
            core::TransformTField field;

            const auto& pos = entry.attr("pos").cast<std::tuple<long, long>>();
            field.x = std::get<0>(pos);
            field.y = std::get<1>(pos);
            field.z = entry.attr("z").cast<double>();

            const auto& layer_size = entry.attr("layer_size").cast<std::tuple<long, long>>();
            field.layer_size = {std::get<0>(layer_size), std::get<1>(layer_size)};

            field.rotation = to_rational(entry.attr("rotation"));

            return field;
        };

        core::TextureTField parse_texture_tfield(const pybind11::object& entry) {
            core::TextureTField field;
            field.uv_flip_v = entry.attr("flip_v").cast<bool>();
            field.uv_flip_h = entry.attr("flip_h").cast<bool>();

            const auto& crop_begin = entry.attr("crop_begin").cast<std::tuple<long, long>>();
            field.crop_begin[0] = std::get<0>(crop_begin);
            field.crop_begin[1] = std::get<1>(crop_begin);

            const auto& crop_end = entry.attr("crop_end").cast<std::tuple<long, long>>();
            field.crop_end[0] = std::get<0>(crop_end);
            field.crop_end[1] = std::get<1>(crop_end);

            return field;
        };

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

        core::ShaderTField parse_shader_tfield(const pybind11::object& entry) {
            core::ShaderTField field;
            field.frag = parse_shader(entry.attr("frag_shader"));
            field.poly = parse_shader(entry.attr("poly_shader"));
            return field;
        }

        static void parse_base_media_tfield(core::BaseMediaTField* field,
                                            const pybind11::object& entry) {
            field->src = entry.attr("req_src").cast<std::string>();
            field->gain = entry.attr("gain").cast<double>();
            field->start = to_rational(entry.attr("start"));
            field->end = to_rational(entry.attr("end"));
        }

        core::VideoTField parse_video_tfield(const pybind11::object& entry) {
            core::VideoTField field;
            parse_base_media_tfield(&field, entry);
            return field;
        }

        core::AudioTField parse_audio_tfield(const pybind11::object& entry) {
            core::AudioTField field;
            parse_base_media_tfield(&field, entry);
            return field;
        }

        core::ImageTField parse_image_tfield(const pybind11::object& entry) {
            core::ImageTField field;
            for (const auto& src : entry.attr("_req_srcs_to_list")().cast<pybind11::list>()) {
                field.srcs.push_back(src.cast<std::string>());
            }
            return field;
        }

        core::TextTField parse_text_tfield(const pybind11::object& entry) {
            core::TextTField field;
            field.text = entry.attr("req_text").cast<std::string>();

            std::string text_align_str = entry.attr("text_align").cast<std::string>();
            if (text_align_str == "center") {
                field.text_align = core::TextAlign::CENTER;
            } else if (text_align_str == "right") {
                field.text_align = core::TextAlign::RIGHT;
            } else {
                field.text_align = core::TextAlign::LEFT;
            }

            auto pads = entry.attr("pad").cast<std::tuple<long, long, long, long>>();
            field.pad[0] = std::get<0>(pads);
            field.pad[1] = std::get<1>(pads);
            field.pad[2] = std::get<2>(pads);
            field.pad[3] = std::get<3>(pads);

            field.line_span = entry.attr("line_span").cast<int32_t>();
            return field;
        }

        core::TextStyleTField parse_text_style_tfield(const pybind11::object& entry) {
            core::TextStyleTField field;
            field.font_path = entry.attr("font_path").cast<std::string>();
            field.fg_size = entry.attr("fg_size").cast<unsigned long>();
            field.fg_color = entry.attr("fg_color").cast<std::string>();
            field.use_outline = entry.attr("use_outline").cast<bool>();
            field.outline_size = entry.attr("outline_size").cast<unsigned long>();
            field.outline_color = entry.attr("outline_color").cast<std::string>();
            field.use_shadow = entry.attr("use_shadow").cast<bool>();
            field.shadow_size = entry.attr("shadow_size").cast<unsigned long>();
            field.shadow_color = entry.attr("shadow_color").cast<std::string>();
            return field;
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

        static void parse_base_shape_tfield(core::BaseShapeTField* field,
                                            const pybind11::object& entry) {
            field->fill_color = entry.attr("fill_color").cast<std::string>();
            field->border_size = entry.attr("border_size").cast<double>();
            field->border_color = entry.attr("border_color").cast<std::string>();
            field->border_direction = parse_border_direction(entry.attr("border_direction"));
            field->edge_radius = entry.attr("edge_radius").cast<double>();
        }

        core::RectTField parse_rect_tfield(const pybind11::object& entry) {
            core::RectTField field;
            parse_base_shape_tfield(&field, entry);
            const auto& req_size = entry.attr("req_size").cast<std::tuple<long, long>>();
            field.width = std::get<0>(req_size);
            field.height = std::get<1>(req_size);
            return field;
        }

        core::CircleTField parse_circle_tfield(const pybind11::object& entry) {
            core::CircleTField field;
            parse_base_shape_tfield(&field, entry);
            return field;
        }

        core::TriTField parse_tri_tfield(const pybind11::object& entry) {
            core::TriTField field;
            parse_base_shape_tfield(&field, entry);
            field.width = entry.attr("width").cast<long>();
            field.height = entry.attr("height").cast<long>();
            field.wr = entry.attr("wr").cast<double>();
            field.hr = entry.attr("hr").cast<double>();
            return field;
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

        core::LineTField parse_line_tfield(const pybind11::object& entry) {
            core::LineTField field;

            field.size = entry.attr("req_size").cast<double>();
            const auto& begin = entry.attr("req_begin").cast<std::tuple<long, long>>();
            const auto& end = entry.attr("req_end").cast<std::tuple<long, long>>();
            field.begin = {std::get<0>(begin), std::get<1>(begin)};
            field.end = {std::get<0>(end), std::get<1>(end)};
            field.style = parse_line_style(entry.attr("style"));

            return field;
        }

        core::UnitTField parse_unit_tfield(const pybind11::object& entry) {
            core::UnitTField field;

            field.layer_indices = entry.attr("layer_indices").cast<std::vector<unsigned long>>();
            field.bg_color = entry.attr("bg_color").cast<std::string>();
            const auto& fb_size = entry.attr("fb_size").cast<std::tuple<long, long>>();
            field.fb_size = {std::get<0>(fb_size), std::get<1>(fb_size)};

            return field;
        }

    }
}
