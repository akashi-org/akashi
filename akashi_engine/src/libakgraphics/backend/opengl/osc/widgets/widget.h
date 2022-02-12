#pragma once

namespace akashi {
    namespace graphics {

        struct RenderParams;
        class OSCRenderContext;
        struct OSCMouseEvent;
        struct OSCTimeEvent;

        namespace osc {

            struct BoundingBox {
                int cx = 0; // center_x
                int cy = 0; // center_y
                int w = 0;
                int h = 0;
            };

            struct CalculatedBoundingBox {
                int left = 0;
                int right = 0;
                int top = 0;
                int bottom = 0;
            };

            class BaseWidget {
              public:
                explicit BaseWidget(OSCRenderContext& render_ctx, const BoundingBox& bbox);

                virtual ~BaseWidget();

                virtual bool update(OSCRenderContext& render_ctx, const RenderParams& params) = 0;

                virtual bool render(OSCRenderContext& render_ctx, const RenderParams& params) = 0;

                BoundingBox bounding_box() const { return m_bbox; }

                CalculatedBoundingBox calculated_bounding_box() const { return m_calc_bbox; }

                void update_bounding_box(const BoundingBox& bbox);

                virtual bool has_collision(int x, int y);

                bool detect_mouse_move() const { return m_detect_mouse_move; }

                bool detect_mouse_hold() const { return m_detect_mouse_hold; }

                bool detect_grab_mouse_move() const { return m_detect_grab_mouse_move; }

                virtual bool on_mouse_event(OSCRenderContext& /*render_ctx*/,
                                            const OSCMouseEvent& /*event*/) {
                    return false;
                };

                virtual bool on_time_event(OSCRenderContext& /*render_ctx*/,
                                           const OSCTimeEvent& /*event*/) {
                    return false;
                };

              protected:
                BoundingBox m_bbox;
                CalculatedBoundingBox m_calc_bbox;
                CalculatedBoundingBox m_collision_offset;
                bool m_detect_mouse_move = false;
                bool m_detect_mouse_hold = false;
                bool m_detect_grab_mouse_move = false;
            };

        }

    }
}
