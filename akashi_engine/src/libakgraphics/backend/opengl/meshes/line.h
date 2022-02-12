#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        class LineMesh final : public BaseMesh {
          public:
            explicit LineMesh() : BaseMesh(){};
            virtual ~LineMesh() = default;

            bool create_default(const GLuint vertices_loc, const GLfloat size,
                                const std::array<long, 2>& begin, const std::array<long, 2>& end);

          protected:
            void destroy_inner() override;

          private:
            bool load_default_line_mesh(const GLuint vertices_loc, const GLfloat size,
                                        const std::array<long, 2>& begin,
                                        const std::array<long, 2>& end);
        };

    }
}
