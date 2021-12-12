#include "./triangle.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <array>

namespace akashi {
    namespace graphics {

        namespace priv {

            static std::vector<GLfloat> tri_mesh_vertices(const GLfloat side) {
                float SQRT3 = sqrt(3);
                // clang-format off
                std::vector<GLfloat> vertices = {
                    0, (SQRT3 / 3.0f) * side, 0, // top
                    -0.5f * side, (SQRT3/ -6.0f) * side, 0, // left
                    0.5f * side, (SQRT3/ -6.0f) * side, 0 // right
                };
                // clang-format on
                return vertices;
            }

        }

        /* Triangle */

        bool TriangleMesh::create(const GLfloat side, const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_tri_mesh(vertices_loc, side));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        bool TriangleMesh::create_border(const GLfloat side, const GLfloat border_width,
                                         const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_tri_border_mesh(vertices_loc, side, border_width));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        void TriangleMesh::destroy() {}

        bool TriangleMesh::load_tri_mesh(const GLuint vertices_loc, GLfloat side) {
            auto vertices = priv::tri_mesh_vertices(side);

            GLuint vertices_vbo;
            create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices.data(),
                          sizeof(GLfloat) * vertices.size());

            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            glEnableVertexAttribArray(vertices_loc);
            glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);

            unsigned short indices[] = {0, 1, 2};

            create_buffer(m_ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
            m_ibo_length = 3;

            return true;
        }

        bool TriangleMesh::load_tri_border_mesh(const GLuint vertices_loc, GLfloat side,
                                                const GLfloat border_width) {
            auto inner_vertices = priv::tri_mesh_vertices(side);
            auto outer_vertices = priv::tri_mesh_vertices(side + border_width);

            size_t vertice_point_length = 3 * 2;

            std::vector<GLfloat> vertices(inner_vertices.size() + outer_vertices.size(), 0);

            size_t v_index = 0;

            for (size_t i = 0; i < inner_vertices.size(); i += 3) {
                vertices[v_index++] = inner_vertices[i];
                vertices[v_index++] = inner_vertices[i + 1];
                vertices[v_index++] = inner_vertices[i + 2];
                vertices[v_index++] = outer_vertices[i];
                vertices[v_index++] = outer_vertices[i + 1];
                vertices[v_index++] = outer_vertices[i + 2];
            }

            GLuint vertices_vbo;
            create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices.data(),
                          sizeof(GLfloat) * vertices.size());

            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            glEnableVertexAttribArray(vertices_loc);
            glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);

            size_t ibo_size = (2 * 3) * 3; // num_of_polygons * num_of_index_points
            size_t ibo_index = 0;
            std::vector<unsigned short> indices(ibo_size, 0);

            auto clip = [vertice_point_length](size_t x) {
                if (x < vertice_point_length) {
                    return x;
                } else {
                    return x % vertice_point_length;
                }
            };

            for (size_t i = 0; i < vertice_point_length; i++) {
                indices[ibo_index++] = clip(i);
                if (i % 2 == 0) {
                    indices[ibo_index++] = clip(i + 1);
                    indices[ibo_index++] = clip(i + 2);
                } else {
                    indices[ibo_index++] = clip(i + 2);
                    indices[ibo_index++] = clip(i + 1);
                }
            }

            create_buffer(m_ibo, GL_ELEMENT_ARRAY_BUFFER, indices.data(),
                          sizeof(unsigned short) * indices.size());
            m_ibo_length = ibo_size;

            return true;
        }

    }
}
