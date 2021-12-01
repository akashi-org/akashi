#include "./quad.h"

#include <libakcore/logger.h>

#include <array>

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
                // auto hsar = (quad_width / quad_height) * 0.5f;
                // GLfloat vertices[] = {
                //     -hsar, 0.5,  0.0, // left-top
                //     hsar,  0.5,  0.0, // right-top
                //     -hsar, -0.5, 0.0, // left-bottom
                //     hsar,  -0.5, 0.0  // right-bottom
                // };

                auto quad_hwidth = quad_width * 0.5f;
                auto quad_hheight = quad_height * 0.5f;
                GLfloat vertices[] = {
                    -quad_hwidth, quad_hheight,  0.0, // left-top
                    quad_hwidth,  quad_hheight,  0.0, // right-top
                    -quad_hwidth, -quad_hheight, 0.0, // left-bottom
                    quad_hwidth,  -quad_hheight, 0.0  // right-bottom
                };

                GLuint vertices_vbo;
                priv::create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices, sizeof(vertices));

                glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
                glEnableVertexAttribArray(vertices_loc);
                glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                      (GLvoid*)0);
            }

            // Expects a suitable vao to be binded before calling this function
            static void load_uvs(const GLuint uvs_loc, const bool flip_uv) {
                std::array<GLfloat, 2 * 4> uvs;
                uvs = {
                    0.0, 0.0, // left-top
                    1.0, 0.0, // right-top
                    0.0, 1.0, // left-bottom
                    1.0, 1.0  // right-bottom
                };

                if (flip_uv) {
                    uvs = {
                        0.0, 1.0, // left-bottom
                        1.0, 1.0, // right-bottom
                        0.0, 0.0, // left-top
                        1.0, 0.0  // right-top
                    };
                }

                GLuint uvs_vbo;
                priv::create_buffer(uvs_vbo, GL_ARRAY_BUFFER, uvs.data(), sizeof(uvs));

                glBindBuffer(GL_ARRAY_BUFFER, uvs_vbo);
                glEnableVertexAttribArray(uvs_loc);
                glVertexAttribPointer(uvs_loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                                      (GLvoid*)0);
            }

            static void load_ibo(GLuint& ibo, size_t& ibo_length) {
                // unsigned short indices[] = {
                //     0, 1, 2, // left
                //     1, 3, 2  // right
                // };
                unsigned short indices[] = {
                    0, 2, 1, // left
                    2, 3, 1  // right
                };

                priv::create_buffer(ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
                ibo_length = 6;
            }

        }

        bool QuadMesh::create(const std::array<float, 2>& size, const GLuint vertices_loc,
                              const GLuint uvs_loc, const bool flip_uv) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            priv::load_vertices(vertices_loc, size[0], size[1]);
            priv::load_uvs(uvs_loc, flip_uv);

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
