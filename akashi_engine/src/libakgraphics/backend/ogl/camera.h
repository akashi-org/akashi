#pragma once

#include <glm/glm.hpp>

namespace akashi {
    namespace graphics {

        struct ProjectionState {
            float fov = 65.0;
            float aspect_ratio = 16.0 / 9.0;
            float near = 0.1;
            float far = 100.0;
        };

        struct ViewState {
            glm::vec3 camera{0, 0, 1};
            glm::vec3 target{0};
            glm::vec3 depth_dir{0, 0, -1}; // camera front or camera direction
            glm::vec3 vert_dir{0, 1, 0};   // camera up
            glm::vec3 horiz_dir;           // camera right (usually calculated)

            float pitch = 0.f; // depth tilt
            float yaw = -90.f; // horiz tilt
        };

        struct FBInfo;

        class Camera final {
          public:
            explicit Camera(ProjectionState* proj_state = nullptr, ViewState* view_state = nullptr);
            virtual ~Camera();

            void update(const FBInfo& fb_info);

            void set_target(const glm::vec3& target);

            const glm::vec3& pos() const { return m_view_state.camera; }

            glm::mat4 vp_mat() const;

          private:
            void update_proj_mat();

            void update_view_mat();

          private:
            ProjectionState m_proj_state;
            ViewState m_view_state;

            glm::mat4 m_proj_mat;
            glm::mat4 m_view_mat;
        };

    }
}
