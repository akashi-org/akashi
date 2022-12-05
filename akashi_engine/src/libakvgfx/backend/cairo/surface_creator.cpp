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
                                     const core::ShapeLayerContext& shape_ctx) {
            return cairo_image_surface_create(
                to_cairo_format(entry.format),
                entry.width + shape_ctx.border_size * entry.border_padv,
                entry.height + shape_ctx.border_size * entry.border_padv);
        }

        static void do_translate_for_border(cairo_t* handle, const SurfaceEntry& entry,
                                            const core::ShapeLayerContext& shape_ctx) {
            const int border_padv = entry.border_padv / 2;
            const double border_padding = shape_ctx.border_size * border_padv;
            cairo_translate(handle, border_padding, border_padding);
        }

        static void do_fill(cairo_t* handle, const core::ShapeLayerContext& shape_ctx) {
            if (!shape_ctx.fill_color.empty()) {
                auto fill_color = core::to_rgba_float(shape_ctx.fill_color);
                cairo_set_source_rgba(handle, fill_color[0], fill_color[1], fill_color[2],
                                      fill_color[3]);
                cairo_fill_preserve(handle);
            }
        }

        static void do_draw_border(cairo_t* handle, const core::ShapeLayerContext& shape_ctx) {
            if (shape_ctx.border_size > 0) {
                auto border_color_str =
                    shape_ctx.border_color.empty() ? "#ffffff" : shape_ctx.border_color;
                if (shape_ctx.border_color.empty()) {
                    AKLOG_WARNN("`border_color` is not set");
                }
                auto border_color = core::to_rgba_float(border_color_str);
                cairo_set_source_rgba(handle, border_color[0], border_color[1], border_color[2],
                                      border_color[3]);
                cairo_set_line_width(handle, shape_ctx.border_size);
                cairo_stroke_preserve(handle);
            }
        }

        static void do_paint(cairo_t* handle, const core::ShapeLayerContext& shape_ctx) {
            switch (shape_ctx.border_direction) {
                case core::BorderDirection::INNER: {
                    cairo_clip_preserve(handle);
                    do_fill(handle, shape_ctx);
                    do_draw_border(handle, shape_ctx);
                    break;
                }
                case core::BorderDirection::OUTER: {
                    do_draw_border(handle, shape_ctx);
                    cairo_set_operator(handle, CAIRO_OPERATOR_SOURCE);
                    do_fill(handle, shape_ctx);
                    cairo_set_operator(handle, CAIRO_OPERATOR_OVER);
                    break;
                }
                case core::BorderDirection::FULL: {
                    do_fill(handle, shape_ctx);
                    do_draw_border(handle, shape_ctx);
                    break;
                }
                default: {
                }
            }
        }

    }
    namespace vgfx {

        static void create_rect_surface(const SurfaceEntry& entry,
                                        const core::ShapeLayerContext& shape_ctx,
                                        cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, shape_ctx);
            auto handle = cairo_create(raw_surface);

            // path
            {
                priv::do_translate_for_border(handle, entry, shape_ctx);

                if (shape_ctx.edge_radius > 0) {
                    double radius = shape_ctx.edge_radius;
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
            priv::do_paint(handle, shape_ctx);

            cairo_destroy(handle);
        }

        static void create_circle_surface(const SurfaceEntry& entry,
                                          const core::ShapeLayerContext& shape_ctx,
                                          cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, shape_ctx);

            auto handle = cairo_create(raw_surface);

            // path
            {
                priv::do_translate_for_border(handle, entry, shape_ctx);

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
            priv::do_paint(handle, shape_ctx);

            cairo_destroy(handle);
        }

        static void create_tri_surface(const SurfaceEntry& entry,
                                       const core::ShapeLayerContext& shape_ctx,
                                       cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, shape_ctx);

            auto handle = cairo_create(raw_surface);
            auto tri = shape_ctx.tri;

            // path
            {
                priv::do_translate_for_border(handle, entry, shape_ctx);

                auto top_y = (entry.height * std::abs(1.0 - tri.hr));

                if (tri.wr < 0) {
                    cairo_move_to(handle, 0, top_y);
                    cairo_line_to(handle, std::abs(tri.wr) * tri.width, entry.height);
                    cairo_rel_line_to(handle, tri.width, 0);
                } else if (tri.wr > 1) {
                    cairo_move_to(handle, entry.width - tri.width, top_y);
                    cairo_line_to(handle, entry.width - (1 + tri.wr) * tri.width, entry.height);
                    cairo_rel_line_to(handle, tri.width, 0);
                } else {
                    cairo_move_to(handle, tri.wr * tri.width, top_y);
                    cairo_line_to(handle, 0, entry.height);
                    cairo_rel_line_to(handle, tri.width, 0);
                }
                cairo_close_path(handle);
            }

            // paint
            priv::do_paint(handle, shape_ctx);

            cairo_destroy(handle);
        }

        static void create_line_surface(const SurfaceEntry& entry,
                                        const core::ShapeLayerContext& shape_ctx,
                                        cairo_surface_t*& raw_surface) {
            raw_surface = priv::create_default_image_surface(entry, shape_ctx);
            auto handle = cairo_create(raw_surface);
            auto line = shape_ctx.line;

            auto fill_color_str = shape_ctx.fill_color.empty() ? "#ffffff" : shape_ctx.fill_color;
            if (shape_ctx.fill_color.empty()) {
                AKLOG_WARNN("`fill_color` is not set");
            }
            auto line_color = core::to_rgba_float(fill_color_str);
            cairo_set_source_rgba(handle, line_color[0], line_color[1], line_color[2],
                                  line_color[3]);
            cairo_set_line_width(handle, line.size);

            cairo_move_to(handle, line.begin[0], line.begin[1]);
            cairo_line_to(handle, line.end[0], line.end[1]);

            cairo_stroke_preserve(handle);

            cairo_destroy(handle);
        }

        core::owned_ptr<vgfx::Surface>
        cairo_create_surface(const SurfaceEntry& entry, const core::ShapeLayerContext& shape_ctx) {
            SurfaceKind surface_kind = SurfaceKind::NONE;
            cairo_surface_t* raw_surface = nullptr;

            switch (shape_ctx.shape_kind) {
                case core::ShapeKind::RECT: {
                    surface_kind = SurfaceKind::RECT;
                    create_rect_surface(entry, shape_ctx, raw_surface);
                    break;
                }
                case core::ShapeKind::CIRCLE: {
                    surface_kind = SurfaceKind::CIRCLE;
                    create_circle_surface(entry, shape_ctx, raw_surface);
                    break;
                }
                case core::ShapeKind::TRIANGLE: {
                    surface_kind = SurfaceKind::TRI;
                    create_tri_surface(entry, shape_ctx, raw_surface);
                    break;
                }
                case core::ShapeKind::LINE: {
                    surface_kind = SurfaceKind::LINE;
                    create_line_surface(entry, shape_ctx, raw_surface);
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid or not implemented shape kind {} found",
                                shape_ctx.shape_kind);
                    break;
                }
            }

            if (!raw_surface) {
                return nullptr;
            }
            return make_owned<CairoSurface>(surface_kind, raw_surface);
        }

    }
}
