#include "./camera.h"

#include "./fbo.h"
#include "./core/glc.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
using namespace glm;

namespace akashi {
    namespace graphics {

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

        void Camera::set_target(const glm::vec3& target) {
            m_view_state.target = target;
            m_view_state.depth_dir = glm::normalize(target - m_view_state.camera);

            auto pitch_angle = glm::asin(m_view_state.depth_dir.y);
            auto yaw_angle = glm::acos(m_view_state.depth_dir.x / cos(pitch_angle));

            m_view_state.pitch = glm::degrees(pitch_angle);
            m_view_state.yaw = -1 * glm::degrees(yaw_angle);

            m_view_state.horiz_dir =
                glm::normalize(glm::cross(m_view_state.vert_dir, m_view_state.depth_dir));
            this->update_view_mat();
        }

        void Camera::update(const FBInfo& /* fb_info */) {
            // auto cur_aspect_ratio = (double)(fb_info.width) / fb_info.height;
            // if ((int)(cur_aspect_ratio * 1000) != (int)(m_proj_state.aspect_ratio * 1000)) {
            //     m_proj_state.aspect_ratio = cur_aspect_ratio;
            //     this->update_proj_mat();
            // }

            // static float x_sign = 1;
            // // move to the right
            // if (m_view_state.camera.x > 1) {
            //     x_sign = -1;
            // } else if (m_view_state.camera.x < -1) {
            //     x_sign = 1;
            // }
            // m_view_state.camera -= x_sign * m_view_state.horiz_dir * (0.7f * 24.0f / 1000.0f);

            // static float y_sign = 1;
            // // move to the top
            // if (m_view_state.camera.y > 1) {
            //     y_sign = 1;
            // } else if (m_view_state.camera.y < -1) {
            //     y_sign = -1;
            // }
            // m_view_state.camera -= y_sign * m_view_state.vert_dir * (0.7f * 24.0f / 1000.0f);

            // static float z_sign = 1;
            // // move to the front
            // if (m_view_state.camera.z > 2) {
            //     z_sign = -1;
            // } else if (m_view_state.camera.z < 0.5) {
            //     z_sign = 1;
            // }
            // m_view_state.camera -= z_sign * m_view_state.depth_dir * (0.3f * 24.0f / 1000.0f);

            // auto pitch_angle = glm::radians(m_view_state.pitch);
            // auto yaw_angle = glm::radians(m_view_state.yaw);

            // m_view_state.depth_dir.x = cos(yaw_angle) * cos(pitch_angle);
            // m_view_state.depth_dir.y = sin(pitch_angle);
            // m_view_state.depth_dir.z = sin(yaw_angle) * cos(pitch_angle);
            // m_view_state.depth_dir = glm::normalize(m_view_state.depth_dir);

            // m_view_state.horiz_dir =
            //     glm::normalize(glm::cross(m_view_state.vert_dir, m_view_state.depth_dir));

            // this->update_view_mat();
        }

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
