#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        class CircleMesh final : public BaseMesh {
          public:
            explicit CircleMesh() : BaseMesh(){};
            virtual ~CircleMesh() = default;

            bool create(const std::array<GLfloat, 2>& size, const int lod,
                        const GLuint vertices_loc);

            bool create_border(const std::array<GLfloat, 2>& size, const int lod,
                               const GLfloat border_width, const GLuint vertices_loc);

          protected:
            void destroy_inner() override;

          private:
            bool load_circle_mesh(const GLuint vertices_loc, const std::array<GLfloat, 2>& size,
                                  int lod);

            bool load_circle_border_mesh(const GLuint vertices_loc,
                                         const std::array<GLfloat, 2>& size, int lod,
                                         const GLfloat border_width);
        };

    }
}
