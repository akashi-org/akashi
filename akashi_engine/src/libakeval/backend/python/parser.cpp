#include "./parser.h"

#include "./core/object.h"
#include "./core/value.h"
#include "./core/string.h"
#include "./core/list.h"
#include "./core/dict.h"

#include <libakcore/common.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>

#include <string>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        static PythonValue get_attr(const core::borrowed_ptr<const PythonObject>& obj,
                                    const char* attr_name) {
            return PythonValue::from(obj->attr(attr_name)->get_raw(), false);
        }

        static PythonValue get_attr(const core::borrowed_ptr<PythonObject>& obj,
                                    const char* attr_name) {
            return PythonValue::from(obj->attr(attr_name)->get_raw(), false);
        }

        template <typename T>
        static bool has_key(const core::borrowed_ptr<T>& obj, const char* key) {
            static_assert(std::is_same<T, const PythonObject>::value ||
                              std::is_same<T, PythonObject>::value,
                          "obj type mismatch");
            return has_key(obj->get_raw(), key);
        }

        template <typename T>
        static PythonValue get_item(const core::borrowed_ptr<T>& obj, const char* key) {
            static_assert(std::is_same<T, const PythonObject>::value ||
                              std::is_same<T, PythonObject>::value,
                          "obj type mismatch");
            return PythonValue::from(get_item(obj->get_raw(), key).get_raw(), false);
        }

        core::RenderProfile parse_renderProfile(core::borrowed_ptr<PythonObject> obj) {
            core::RenderProfile render_prof;

            render_prof.duration = parse_second(borrowed_ptr(obj->attr("duration")));
            render_prof.uuid = get_attr(obj, "uuid").to<PythonString>().char_p();
            render_prof.atom_profiles = map_list<AtomProfile>(
                obj->attr("atom_profiles")->get_raw(),
                [](borrowed_ptr<const PythonObject> pyo) { return parse_atomProfile(pyo); });

            return render_prof;
        }

        core::AtomProfile parse_atomProfile(core::borrowed_ptr<const PythonObject> obj) {
            core::AtomProfile atom_prof;

            atom_prof.from = parse_second(borrowed_ptr(obj->attr("begin")));
            atom_prof.to = parse_second(borrowed_ptr(obj->attr("end")));
            atom_prof.duration = parse_second(borrowed_ptr(obj->attr("duration")));
            atom_prof.uuid = get_attr(obj, "uuid").to<PythonString>().char_p();
            atom_prof.layers = map_list<LayerProfile>(
                obj->attr("layers")->get_raw(),
                [](borrowed_ptr<const PythonObject> pyo) { return parse_layerProfile(pyo); });

            return atom_prof;
        }

        core::LayerProfile parse_layerProfile(core::borrowed_ptr<const PythonObject> obj) {
            core::LayerProfile layer_prof;

            layer_prof.from = parse_second(borrowed_ptr(obj->attr("begin")));
            layer_prof.to = parse_second(borrowed_ptr(obj->attr("end")));
            layer_prof.uuid = get_attr(obj, "uuid").to<PythonString>().char_p();
            layer_prof.src = get_attr(obj, "src").to<PythonString>().char_p();
            layer_prof.start = parse_second(borrowed_ptr(obj->attr("start")));

            std::string type_str = get_attr(obj, "type").to<PythonString>().char_p();
            if (type_str == "VIDEO") {
                layer_prof.type = LayerType::VIDEO;
            } else if (type_str == "AUDIO") {
                layer_prof.type = LayerType::AUDIO;
            } else {
                AKLOG_ERROR("parse_layerProfile(): Invalid type '{}' found", type_str.c_str());
                layer_prof.type = LayerType::LENGTH;
            }

            return layer_prof;
        }

        core::FrameContext parse_frameContext(core::borrowed_ptr<const PythonObject> obj) {
            core::FrameContext frame_ctx;

            frame_ctx.pts = parse_second(borrowed_ptr(obj->attr("pts")));
            frame_ctx.layer_ctxs = map_list<LayerContext>(
                obj->attr("layer_ctxs")->get_raw(),
                [](borrowed_ptr<const PythonObject> pyo) { return parse_layerContext(pyo); });
            return frame_ctx;
        }

        template <typename T>
        core::FrameContext parse_frameContext(core::borrowed_ptr<T> obj) {
            core::FrameContext frame_ctx;

            frame_ctx.pts = parse_second(borrowed_ptr(obj->attr("pts")));
            frame_ctx.layer_ctxs = map_list<LayerContext>(
                obj->attr("layer_ctxs")->get_raw(),
                [](borrowed_ptr<const PythonObject> pyo) { return parse_layerContext(pyo); });
            return frame_ctx;
        }

        template core::FrameContext
        parse_frameContext<const PythonObject>(core::borrowed_ptr<const PythonObject> obj);

        template core::FrameContext
        parse_frameContext<PythonObject>(core::borrowed_ptr<PythonObject> obj);

        core::LayerContext parse_layerContext(core::borrowed_ptr<const PythonObject> obj) {
            core::LayerContext layer_ctx;

            layer_ctx.x = get_attr(obj, "x").to<double>();
            layer_ctx.y = get_attr(obj, "y").to<double>();
            layer_ctx.from = parse_second(borrowed_ptr(obj->attr("begin")));
            layer_ctx.to = parse_second(borrowed_ptr(obj->attr("end")));

            std::string type_str = get_attr(obj, "_type").to<PythonString>().char_p();
            if (type_str == "VIDEO") {
                layer_ctx.type = static_cast<int>(LayerType::VIDEO);
            } else if (type_str == "AUDIO") {
                layer_ctx.type = static_cast<int>(LayerType::AUDIO);
            } else if (type_str == "IMAGE") {
                layer_ctx.type = static_cast<int>(LayerType::IMAGE);
            } else if (type_str == "TEXT") {
                layer_ctx.type = static_cast<int>(LayerType::TEXT);
            } else {
                AKLOG_ERROR("parse_layerContext(): Invalid type '{}' found", type_str.c_str());
                layer_ctx.type = -1;
            }

            layer_ctx.uuid = get_attr(obj, "_uuid").to<PythonString>().char_p();
            layer_ctx.atom_uuid = get_attr(obj, "_atom_uuid").to<PythonString>().char_p();
            layer_ctx.display = get_attr(obj, "_display").to<bool>();

            // [TODO] audio?
            switch (static_cast<LayerType>(layer_ctx.type)) {
                case LayerType::VIDEO: {
                    VideoLayerContext layer_ctx_;
                    layer_ctx_.src = get_attr(obj, "src").to<PythonString>().char_p();
                    layer_ctx_.start = parse_second(borrowed_ptr(obj->attr("start")));
                    layer_ctx_.scale = get_attr(obj, "scale").to<double>();
                    if (obj->has_attr("frag_path")) {
                        layer_ctx_.frag_path[json::optional::attr_name] =
                            get_attr(obj, "frag_path").to<PythonString>().char_p();
                    }
                    if (obj->has_attr("geom_path")) {
                        layer_ctx_.geom_path[json::optional::attr_name] =
                            get_attr(obj, "geom_path").to<PythonString>().char_p();
                    }
                    layer_ctx.video_layer_ctx = layer_ctx_;
                    break;
                }
                case LayerType::TEXT: {
                    TextLayerContext layer_ctx_;
                    layer_ctx_.text = get_attr(obj, "text").to<PythonString>().char_p();
                    layer_ctx_.scale = get_attr(obj, "scale").to<double>();
                    if (obj->has_attr("style")) {
                        layer_ctx_.style[json::optional::attr_name] =
                            parse_style(borrowed_ptr(obj->attr("style")));
                    }
                    if (obj->has_attr("frag_path")) {
                        layer_ctx_.frag_path[json::optional::attr_name] =
                            get_attr(obj, "frag_path").to<PythonString>().char_p();
                    }
                    if (obj->has_attr("geom_path")) {
                        layer_ctx_.geom_path[json::optional::attr_name] =
                            get_attr(obj, "geom_path").to<PythonString>().char_p();
                    }
                    layer_ctx.text_layer_ctx = layer_ctx_;
                    break;
                }
                case LayerType::IMAGE: {
                    ImageLayerContext layer_ctx_;
                    layer_ctx_.src = get_attr(obj, "src").to<PythonString>().char_p();
                    layer_ctx_.scale = get_attr(obj, "scale").to<double>();
                    if (obj->has_attr("frag_path")) {
                        layer_ctx_.frag_path[json::optional::attr_name] =
                            get_attr(obj, "frag_path").to<PythonString>().char_p();
                    }
                    if (obj->has_attr("geom_path")) {
                        layer_ctx_.geom_path[json::optional::attr_name] =
                            get_attr(obj, "geom_path").to<PythonString>().char_p();
                    }
                    layer_ctx.image_layer_ctx = layer_ctx_;
                    break;
                }
                default: {
                }
            }

            return layer_ctx;
        }

        core::Fraction parse_second(core::borrowed_ptr<PythonObject> obj) {
            core::Fraction frac;
            frac.num = get_attr(obj, "numerator").to<long long>();
            frac.den = get_attr(obj, "denominator").to<long long>();
            return frac;
        }

        core::Style parse_style(core::borrowed_ptr<PythonObject> obj) {
            core::Style style;

            if (has_key(obj, "font_size")) {
                style.font_size[json::optional::attr_name] =
                    get_item(obj, "font_size").to<unsigned long>();
            }
            if (has_key(obj, "font_path")) {
                style.font_path[json::optional::attr_name] =
                    get_item(obj, "font_path").to<PythonString>().char_p();
            }
            if (has_key(obj, "fill")) {
                style.fill[json::optional::attr_name] =
                    get_item(obj, "fill").to<PythonString>().char_p();
            }
            if (has_key(obj, "color")) {
                style.color[json::optional::attr_name] =
                    get_item(obj, "color").to<PythonString>().char_p();
            }

            return style;
        }

    }
}
