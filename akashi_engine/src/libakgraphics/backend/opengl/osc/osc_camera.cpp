#include "./osc_camera.h"

#include "../core/glc.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
using namespace glm;

namespace akashi {
    namespace graphics::osc {

        Camera::Camera(const ProjectionState& proj_state, ViewState* view_state) {
            m_proj_state = proj_state;
            if (view_state) {
                m_view_state = *view_state;
            }

            m_view_state.depth_dir = glm::normalize(m_view_state.target - m_view_state.camera);
            auto pitch_angle = glm::asin(m_view_state.depth_dir.y);
            auto yaw_angle = glm::acos(m_view_state.depth_dir.x / cos(pitch_angle));

            m_view_state.pitch = glm::degrees(pitch_angle);
            m_view_state.yaw = -1 * glm::degrees(yaw_angle);

            m_view_state.horiz_dir =
                glm::normalize(glm::cross(m_view_state.vert_dir, m_view_state.depth_dir));

            this->update_proj_mat();
            this->update_view_mat();
        }

        Camera::~Camera() {}

        glm::mat4 Camera::vp_mat() const { return m_proj_mat * m_view_mat; }

        void Camera::update_proj_mat() {
            m_proj_mat = glm::ortho<float>(
                m_proj_state.video_width * -0.5f, m_proj_state.video_width * 0.5f,
                m_proj_state.video_height * -0.5f, m_proj_state.video_height * 0.5f,
                m_proj_state.near, m_proj_state.far);
        }

        void Camera::update_view_mat() {
            m_view_mat =
                glm::lookAt(m_view_state.camera, m_view_state.camera + m_view_state.depth_dir,
                            m_view_state.vert_dir);
        }

    }
}
