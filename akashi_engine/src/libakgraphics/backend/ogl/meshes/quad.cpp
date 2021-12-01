#include "./quad.h"

namespace akashi {
    namespace graphics {

        namespace priv {

            static void create_buffer(GLuint& buffer, GLenum target, void* data, size_t data_size) {
                glGenBuffers(1, &buffer);
                glBindBuffer(target, buffer);
                glBufferData(target, data_size, data, GL_STATIC_DRAW);
                glBindBuffer(target, 0);
            }

            // Expects a suitable vao to be binded before calling this function
            static void load_vertices(const GLuint vertices_loc, const GLfloat quad_width,
                                      const GLfloat quad_height) {
                GLfloat vertices[] = {
                    -quad_width, quad_height,  0.0, // left-top
                    quad_width,  quad_height,  0.0, // right-top
                    -quad_width, -quad_height, 0.0, // left-bottom
                    quad_width,  -quad_height, 0.0  // right-bottom
                };
                GLuint vertices_vbo;
                priv::create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices, sizeof(vertices));

                glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
                glEnableVertexAttribArray(vertices_loc);
                glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                      (GLvoid*)0);
            }

            // Expects a suitable vao to be binded before calling this function
            static void load_uvs(const GLuint uvs_loc) {
                GLfloat uvs[] = {
                    0.0, 0.0, // left-top
                    1.0, 0.0, // right-top
                    0.0, 1.0, // left-bottom
                    1.0, 1.0, // right-bottom
                };
                GLuint uvs_vbo;
                priv::create_buffer(uvs_vbo, GL_ARRAY_BUFFER, uvs, sizeof(uvs));

                glBindBuffer(GL_ARRAY_BUFFER, uvs_vbo);
                glEnableVertexAttribArray(uvs_loc);
                glVertexAttribPointer(uvs_loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                      (GLvoid*)0);
            }

            static void load_ibo(GLuint& ibo, size_t& ibo_length) {
                unsigned short indices[] = {
                    0, 1, 2, // left
                    1, 3, 2  // right
                };
                priv::create_buffer(ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
                ibo_length = 6;
            }

        }

        bool QuadMesh::create(const GLuint vertices_loc, const GLuint uvs_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            priv::load_vertices(vertices_loc, 1.0f, 1.0f);
            priv::load_uvs(uvs_loc);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            // load ibo
            priv::load_ibo(m_ibo, m_ibo_length);

            return true;
        }

        void QuadMesh::destroy() {
            glDeleteBuffers(1, &m_ibo);
            glDeleteVertexArrays(1, &m_vao);
            m_ibo_length = 0;
        }

    }
}
