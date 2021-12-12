#include "./line.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>
#include <libakcore/rational.h>

#include <array>

#include <glm/glm.hpp>
using namespace glm;

namespace akashi {
    namespace graphics {

        namespace priv {}

        /* Line */

        bool LineMesh::create_default(const GLuint vertices_loc, const GLfloat size,
                                      const std::array<long, 2>& begin,
                                      const std::array<long, 2>& end) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_default_line_mesh(vertices_loc, size, begin, end));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        void LineMesh::destroy() {}

        bool LineMesh::load_default_line_mesh(const GLuint vertices_loc, const GLfloat size,
                                              const std::array<long, 2>& begin,
                                              const std::array<long, 2>& end) {
            auto p_begin = glm::vec2(begin[0], -begin[1]);
            auto p_end = glm::vec2(end[0], -end[1]);
            auto line_vec = glm::normalize(p_end - p_begin);
            auto normal_vec = glm::vec2(line_vec.y * size * 0.5, -line_vec.x * size * 0.5);

            // clang-format off
            GLfloat vertices[] = {
                p_begin[0] + normal_vec[0], p_begin[1] + normal_vec[1],  0.0,
                p_begin[0] - normal_vec[0], p_begin[1] - normal_vec[1],  0.0, 
                p_end[0] + normal_vec[0], p_end[1] + normal_vec[1],  0.0, 
                p_end[0] - normal_vec[0], p_end[1] - normal_vec[1],  0.0 
            };
            // clang-format on

            GLuint vertices_vbo;
            create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices, sizeof(vertices));

            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            glEnableVertexAttribArray(vertices_loc);
            glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);

            unsigned short indices[] = {2, 1, 0, 3, 1, 2};

            create_buffer(m_ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
            m_ibo_length = 6;

            return true;
        }

    }
}
