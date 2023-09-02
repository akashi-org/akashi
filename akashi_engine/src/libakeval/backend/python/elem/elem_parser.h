#pragma once

#include <libakcore/element.h>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        core::LayerContext parse_layer_context(const GlobalContext& ctx,
                                               const pybind11::object& layer_params);

        core::TransformTField parse_transform_tfield(const pybind11::object& entry);

        core::TextureTField parse_texture_tfield(const pybind11::object& entry);

        core::ShaderTField parse_shader_tfield(const pybind11::object& entry);

        core::VideoTField parse_video_tfield(const pybind11::object& entry);

        core::AudioTField parse_audio_tfield(const pybind11::object& entry);

        core::ImageTField parse_image_tfield(const pybind11::object& entry);

        core::TextTField parse_text_tfield(const pybind11::object& entry);

        core::TextStyleTField parse_text_style_tfield(const pybind11::object& entry);

        core::RectTField parse_rect_tfield(const pybind11::object& entry);

        core::CircleTField parse_circle_tfield(const pybind11::object& entry);

        core::TriTField parse_tri_tfield(const pybind11::object& entry);

        core::LineTField parse_line_tfield(const pybind11::object& entry);

        core::UnitTField parse_unit_tfield(const pybind11::object& entry);

    }
}
