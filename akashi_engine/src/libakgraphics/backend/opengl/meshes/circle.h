#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        class CircleMesh final : public BaseMesh {
          public:
            explicit CircleMesh() : BaseMesh(){};
            virtual ~CircleMesh() = default;

            bool create(const GLfloat radius, const int lod, const GLuint vertices_loc);

            bool create_border(const GLfloat radius, const int lod, const GLfloat border_width,
                               const GLuint vertices_loc);

            void destroy() override;

          private:
            bool load_circle_mesh(const GLuint vertices_loc, GLfloat radius, int lod);

            bool load_circle_border_mesh(const GLuint vertices_loc, GLfloat radius, int lod,
                                         const GLfloat border_width);
        };

    }
}
