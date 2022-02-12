#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        class TriangleMesh final : public BaseMesh {
          public:
            explicit TriangleMesh() : BaseMesh(){};
            virtual ~TriangleMesh() = default;

            bool create(const std::array<GLfloat, 2>& size, const GLuint vertices_loc);

            bool create_border(const std::array<GLfloat, 2>& size, const GLfloat border_width,
                               const GLuint vertices_loc);

          protected:
            void destroy_inner() override;

          private:
            bool load_tri_mesh(const GLuint vertices_loc, const std::array<GLfloat, 2>& size);

            bool load_tri_border_mesh(const GLuint vertices_loc, const std::array<GLfloat, 2>& size,
                                      const GLfloat border_width);
        };

    }
}
