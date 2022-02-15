#include "./triangle.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <array>

namespace akashi {
    namespace graphics {

        namespace priv {

            static std::vector<GLfloat> tri_mesh_vertices(const std::array<GLfloat, 2>& size,
                                                          const GLfloat border_width) {
                // clang-format off
                std::vector<GLfloat> vertices = {
                    0, 0.5f * (size[1] + border_width), 0, // top
                    -0.5f * (size[0] + border_width), -0.5f * (size[1] + border_width), 0, // left
                    0.5f * (size[0] + border_width), -0.5f  * (size[1] + border_width), 0 // right
                };
                // clang-format on
                return vertices;
            }

        }

        /* Triangle */

        bool TriangleMesh::create(const std::array<GLfloat, 2>& size, const GLuint vertices_loc) {
            m_mesh_size = size;

            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_tri_mesh(vertices_loc, size));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        bool TriangleMesh::create_border(const std::array<GLfloat, 2>& size,
                                         const GLfloat border_width, const GLuint vertices_loc) {
            m_mesh_size = {size[0] + (border_width), size[1] + (border_width)};

            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_tri_border_mesh(vertices_loc, size, border_width));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        void TriangleMesh::destroy_inner() {}

        bool TriangleMesh::load_tri_mesh(const GLuint vertices_loc,
                                         const std::array<GLfloat, 2>& size) {
            auto vertices = priv::tri_mesh_vertices(size, 0);

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

        bool TriangleMesh::load_tri_border_mesh(const GLuint vertices_loc,
                                                const std::array<GLfloat, 2>& size,
                                                const GLfloat border_width) {
            auto inner_vertices = priv::tri_mesh_vertices(size, 0);
            auto outer_vertices = priv::tri_mesh_vertices(size, border_width);

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
