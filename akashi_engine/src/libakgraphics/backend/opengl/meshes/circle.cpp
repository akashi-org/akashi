#include "./circle.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <array>

namespace akashi {
    namespace graphics {

        namespace priv {

            static std::vector<GLfloat> circle_mesh_vertices(const std::array<GLfloat, 2>& size,
                                                             const GLfloat border_width,
                                                             const int lod) {
                std::vector<GLfloat> vertices(lod * 3, 0);

                auto delta = (2 * M_PI) / lod;
                size_t v_index = 0;
                for (int i = 0; i < lod; i++) {
                    auto rad = delta * i;
                    vertices[v_index++] = (double)cos(rad) * ((size[0] * 0.5f) + border_width); // x
                    vertices[v_index++] = (double)sin(rad) * ((size[1] * 0.5f) + border_width); // y
                    vertices[v_index++] = 0;                                                    // z
                }

                return vertices;
            }

        }

        /* Circle */

        bool CircleMesh::create(const std::array<GLfloat, 2>& size, const int lod,
                                const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_circle_mesh(vertices_loc, size, lod));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        bool CircleMesh::create_border(const std::array<GLfloat, 2>& size, const int lod,
                                       const GLfloat border_width, const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(this->load_circle_border_mesh(vertices_loc, size, lod, border_width));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        void CircleMesh::destroy() {}

        bool CircleMesh::load_circle_mesh(const GLuint vertices_loc,
                                          const std::array<GLfloat, 2>& size, int lod) {
            std::array<GLfloat, 3> center_pos = {0.0, 0.0, 0.0};

            size_t vertice_point_length = lod + 1;
            std::vector<GLfloat> vertices(vertice_point_length * 3, 0);

            vertices[0] = center_pos[0];
            vertices[1] = center_pos[1];
            vertices[2] = center_pos[2];

            auto part_vertices = priv::circle_mesh_vertices(size, 0, lod);
            for (size_t i = 0; i < part_vertices.size(); i++) {
                vertices[3 + i] = part_vertices[i];
            }

            GLuint vertices_vbo;
            create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices.data(),
                          sizeof(GLfloat) * vertices.size());

            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            glEnableVertexAttribArray(vertices_loc);
            glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);

            size_t ibo_size = lod * 3;
            auto ibo_index = 0;
            std::vector<unsigned short> indices(ibo_size, 0);

            for (size_t i = 1; i < vertice_point_length; i++) {
                if (i == vertice_point_length - 1) {
                    indices[ibo_index++] = 0;
                    indices[ibo_index++] = i;
                    indices[ibo_index++] = 1;
                } else {
                    indices[ibo_index++] = 0;
                    indices[ibo_index++] = i;
                    indices[ibo_index++] = i + 1;
                }
            }

            create_buffer(m_ibo, GL_ELEMENT_ARRAY_BUFFER, indices.data(),
                          sizeof(unsigned short) * indices.size());
            m_ibo_length = ibo_size;

            return true;
        }

        bool CircleMesh::load_circle_border_mesh(const GLuint vertices_loc,
                                                 const std::array<GLfloat, 2>& size, int lod,
                                                 const GLfloat border_width) {
            auto inner_vertices = priv::circle_mesh_vertices(size, 0, lod);
            auto outer_vertices = priv::circle_mesh_vertices(size, border_width, lod);

            size_t vertice_point_length = lod * 2;

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

            size_t ibo_size = (2 * lod) * 3; // num_of_polygons * num_of_index_points
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
