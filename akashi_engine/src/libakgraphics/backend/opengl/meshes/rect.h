#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        class RectMesh final : public BaseMesh {
          public:
            explicit RectMesh() : BaseMesh(){};
            virtual ~RectMesh() = default;

            bool create(const std::array<float, 2>& size, const GLuint vertices_loc);

            bool create_border(const std::array<float, 2>& size, const GLfloat border_width,
                               const GLuint vertices_loc);

            void destroy() override;
        };

    }
}
