#include "./interface.h"
#include "./utils.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/color.h>

using namespace akashi::core;

namespace akashi {
    namespace vgfx::priv {

        static cairo_surface_t*
        create_default_image_surface(const SurfaceEntry& entry,
                                     const core::BaseShapeTField& shape_field) {
            return cairo_image_surface_create(
                to_cairo_format(entry.format),
                entry.width + shape_field.border_size * entry.border_padv,
                entry.height + shape_field.border_size * entry.border_padv);
        }

        static void do_translate_for_border(cairo_t* handle, const SurfaceEntry& entry,
                                            const core::BaseShapeTField& shape_field) {
            const int border_padv = entry.border_padv / 2;
            const double border_padding = shape_field.border_size * border_padv;
            cairo_translate(handle, border_padding, border_padding);
        }

        static void do_fill(cairo_t* handle, const core::BaseShapeTField& shape_field) {
            if (!shape_field.fill_color.empty()) {
                auto fill_color = core::to_rgba_float(shape_field.fill_color);
                cairo_set_source_rgba(handle, fill_color[0], fill_color[1], fill_color[2],
                                      fill_color[3]);
                cairo_fill_preserve(handle);
            }
        }

        static void do_draw_border(cairo_t* handle, const core::BaseShapeTField& shape_field) {
            if (shape_field.border_size > 0) {
                auto border_color_str =
                    shape_field.border_color.empty() ? "#ffffff" : shape_field.border_color;
                // if (shape_field.border_color.empty()) {
                //     AKLOG_WARNN("`border_color` is not set");
                // }
                auto border_color = core::to_rgba_float(border_color_str);
                cairo_set_source_rgba(handle, border_color[0], border_color[1], border_color[2],
                                      border_color[3]);
                cairo_set_line_width(handle, shape_field.border_size);
                cairo_stroke_preserve(handle);
            }
        }

        static void do_paint(cairo_t* handle, const core::BaseShapeTField& shape_field) {
            switch (shape_field.border_direction) {
                case core::BorderDirection::INNER: {
                    cairo_clip_preserve(handle);
                    do_fill(handle, shape_field);
                    do_draw_border(handle, shape_field);
                    break;
                }
                case core::BorderDirection::OUTER: {
                    do_draw_border(handle, shape_field);
                    cairo_set_operator(handle, CAIRO_OPERATOR_SOURCE);
                    do_fill(handle, shape_field);
                    cairo_set_operator(handle, CAIRO_OPERATOR_OVER);
                    break;
                }
                case core::BorderDirection::FULL: {
                    do_fill(handle, shape_field);
                    do_draw_border(handle, shape_field);
                    break;
                }
                default: {
                }
            }
        }

    }
    namespace vgfx {

        static void create_rect_surface(const SurfaceEntry& entry,
                                        const core::RectTField& rect_field,
                                        cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, rect_field);
            auto handle = cairo_create(raw_surface);

            // path
            {
                priv::do_translate_for_border(handle, entry, rect_field);

                if (rect_field.edge_radius > 0) {
                    double radius = rect_field.edge_radius;
                    double degrees = M_PI / 180.0;
                    cairo_new_sub_path(handle);
                    cairo_arc(handle, entry.width - radius, radius, radius, -90 * degrees,
                              0 * degrees);
                    cairo_arc(handle, entry.width - radius, entry.height - radius, radius,
                              0 * degrees, 90 * degrees);
                    cairo_arc(handle, radius, entry.height - radius, radius, 90 * degrees,
                              180 * degrees);
                    cairo_arc(handle, radius, radius, radius, 180 * degrees, 270 * degrees);
                    cairo_close_path(handle);

                } else {
                    cairo_rectangle(handle, 0, 0, entry.width, entry.height);
                }
            }

            // paint
            priv::do_paint(handle, rect_field);

            cairo_destroy(handle);
        }

        static void create_circle_surface(const SurfaceEntry& entry,
                                          const core::CircleTField& circle_field,
                                          cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, circle_field);

            auto handle = cairo_create(raw_surface);

            // path
            {
                priv::do_translate_for_border(handle, entry, circle_field);

                cairo_translate(handle, entry.width * 0.5, entry.height * 0.5);

                double scale[] = {1, 1};
                if (entry.width > entry.height) {
                    scale[1] = (1.0 * entry.height) / entry.width;
                } else {
                    scale[0] = (1.0 * entry.width) / entry.height;
                }
                cairo_scale(handle, scale[0], scale[1]);

                cairo_arc(handle, 0, 0, std::max(entry.width, entry.height) * 0.5, 0, 2 * M_PI);
            }

            // paint
            priv::do_paint(handle, circle_field);

            cairo_destroy(handle);
        }

        static void create_tri_surface(const SurfaceEntry& entry, const core::TriTField& tri_field,
                                       cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, tri_field);

            auto handle = cairo_create(raw_surface);

            // path
            {
                priv::do_translate_for_border(handle, entry, tri_field);

                auto top_y = (entry.height * std::abs(1.0 - tri_field.hr));

                if (tri_field.wr < 0) {
                    cairo_move_to(handle, 0, top_y);
                    cairo_line_to(handle, std::abs(tri_field.wr) * tri_field.width, entry.height);
                    cairo_rel_line_to(handle, tri_field.width, 0);
                } else if (tri_field.wr > 1) {
                    cairo_move_to(handle, entry.width - tri_field.width, top_y);
                    cairo_line_to(handle, entry.width - (1 + tri_field.wr) * tri_field.width,
                                  entry.height);
                    cairo_rel_line_to(handle, tri_field.width, 0);
                } else {
                    cairo_move_to(handle, tri_field.wr * tri_field.width, top_y);
                    cairo_line_to(handle, 0, entry.height);
                    cairo_rel_line_to(handle, tri_field.width, 0);
                }
                cairo_close_path(handle);
            }

            // paint
            priv::do_paint(handle, tri_field);

            cairo_destroy(handle);
        }

        static void create_line_surface(const SurfaceEntry& entry,
                                        const core::LineTField& line_field,
                                        cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, line_field);
            auto handle = cairo_create(raw_surface);

            auto fill_color_str = line_field.fill_color.empty() ? "#ffffff" : line_field.fill_color;
            // if (line_field.fill_color.empty()) {
            //     AKLOG_WARNN("`fill_color` is not set");
            // }
            auto line_color = core::to_rgba_float(fill_color_str);
            cairo_set_source_rgba(handle, line_color[0], line_color[1], line_color[2],
                                  line_color[3]);
            cairo_set_line_width(handle, line_field.size);

            cairo_move_to(handle, line_field.begin[0], line_field.begin[1]);
            cairo_line_to(handle, line_field.end[0], line_field.end[1]);

            cairo_stroke_preserve(handle);

            cairo_destroy(handle);
        }

        core::owned_ptr<vgfx::Surface> cairo_create_surface(const SurfaceEntry& entry,
                                                            const core::LayerContext& layer_ctx) {
            SurfaceKind surface_kind = SurfaceKind::NONE;
            cairo_surface_t* raw_surface = nullptr;

            if (layer_ctx.t_rect) {
                surface_kind = SurfaceKind::RECT;
                create_rect_surface(entry, *layer_ctx.t_rect, raw_surface);
            } else if (layer_ctx.t_circle) {
                surface_kind = SurfaceKind::CIRCLE;
                create_circle_surface(entry, *layer_ctx.t_circle, raw_surface);
            } else if (layer_ctx.t_tri) {
                surface_kind = SurfaceKind::TRI;
                create_tri_surface(entry, *layer_ctx.t_tri, raw_surface);
            } else if (layer_ctx.t_line) {
                surface_kind = SurfaceKind::LINE;
                create_line_surface(entry, *layer_ctx.t_line, raw_surface);
            }

            if (!raw_surface) {
                return nullptr;
            }
            return make_owned<CairoSurface>(surface_kind, raw_surface);
        }

    }
}
