#include "./widget.h"

#include "../../../../item.h"

#include <libakcore/logger.h>

namespace akashi {
    namespace graphics {

        namespace osc {

            static CalculatedBoundingBox get_calc_bbox(const BoundingBox& bbox) {
                CalculatedBoundingBox calc_bbox;
                calc_bbox.left = std::max(0, int(bbox.cx - bbox.w / 2));
                calc_bbox.right = std::max(0, int(bbox.cx + bbox.w / 2));
                calc_bbox.top = std::max(0, int(bbox.cy - bbox.h / 2));
                calc_bbox.bottom = std::max(0, int(bbox.cy + bbox.h / 2));
                return calc_bbox;
            }

            BaseWidget::BaseWidget(OSCRenderContext& /*render_ctx*/, const BoundingBox& bbox)
                : m_bbox(bbox), m_calc_bbox(get_calc_bbox(bbox)) {}

            BaseWidget::~BaseWidget() {}

            void BaseWidget::update_bounding_box(const BoundingBox& bbox) {
                m_bbox = bbox;
                m_calc_bbox = get_calc_bbox(bbox);
            }

            bool BaseWidget::has_collision(int x, int y) {
                // clang-format off
                return (
                    m_calc_bbox.left - m_collision_offset.left <= x && x <= m_calc_bbox.right + m_collision_offset.right && m_calc_bbox.top - m_collision_offset.top <= y && y <= m_calc_bbox.bottom + m_collision_offset.bottom
                );
                // clang-format on
            }

        }

    }
}
