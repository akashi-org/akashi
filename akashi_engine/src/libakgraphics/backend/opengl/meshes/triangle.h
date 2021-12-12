#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        class TriangleMesh final : public BaseMesh {
          public:
            explicit TriangleMesh() : BaseMesh(){};
            virtual ~TriangleMesh() = default;

            bool create(const GLfloat side, const GLuint vertices_loc);

            bool create_border(const GLfloat side, const GLfloat border_width,
                               const GLuint vertices_loc);

            void destroy() override;

          private:
            bool load_tri_mesh(const GLuint vertices_loc, GLfloat side);

            bool load_tri_border_mesh(const GLuint vertices_loc, GLfloat side,
                                      const GLfloat border_width);
        };

    }
}
