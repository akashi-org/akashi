#include "./rect.h"

#include <libakcore/logger.h>

#include <array>

namespace akashi {
    namespace graphics {

        namespace priv {

            // Expects a suitable vao to be binded before calling this function
            static void load_vertices(const GLuint vertices_loc, const GLfloat width,
                                      const GLfloat height) {
                auto hwidth = width * 0.5f;
                auto hheight = height * 0.5f;
                GLfloat vertices[] = {
                    -hwidth, hheight,  0.0, // left-top
                    hwidth,  hheight,  0.0, // right-top
                    -hwidth, -hheight, 0.0, // left-bottom
                    hwidth,  -hheight, 0.0  // right-bottom
                };

                GLuint vertices_vbo;
                create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices, sizeof(vertices));

                glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
                glEnableVertexAttribArray(vertices_loc);
                glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                      (GLvoid*)0);
            }

            static void load_ibo(GLuint& ibo, size_t& ibo_length) {
                unsigned short indices[] = {
                    0, 2, 1, // left
                    2, 3, 1  // right
                };

                create_buffer(ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
                ibo_length = 6;
            }

            // Expects a suitable vao to be binded before calling this function
            static void load_border_vertices(const GLuint vertices_loc, const GLfloat width,
                                             const GLfloat height, const GLfloat bwidth) {
                auto hwidth = width * 0.5f;
                auto hheight = height * 0.5f;
                GLfloat vertices[] = {
                    -hwidth - bwidth, hheight + bwidth,  0.0, // left-top
                    -hwidth,          hheight,           0.0, // left-top-inner
                    -hwidth - bwidth, -hheight - bwidth, 0.0, // left-bottom
                    -hwidth,          -hheight,          0.0, // left-bottom-inner
                    hwidth + bwidth,  hheight + bwidth,  0.0, // right-top
                    hwidth,           hheight,           0.0, // right-top-inner
                    hwidth + bwidth,  -hheight - bwidth, 0.0, // right-bottom
                    hwidth,           -hheight,          0.0  // right-bottom-inner
                };

                GLuint vertices_vbo;
                create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices, sizeof(vertices));

                glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
                glEnableVertexAttribArray(vertices_loc);
                glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                      (GLvoid*)0);
            }

            static void load_border_ibo(GLuint& ibo, size_t& ibo_length) {
                unsigned short indices[] = {0, 2, 1, 1, 2, 3, 1, 4, 0, 4, 1, 5,
                                            7, 4, 5, 4, 7, 6, 2, 7, 3, 7, 2, 6};

                create_buffer(ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
                ibo_length = 24;
            }

        }

        bool RectMesh::create(const std::array<float, 2>& size, const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            priv::load_vertices(vertices_loc, size[0], size[1]);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            // load ibo
            priv::load_ibo(m_ibo, m_ibo_length);

            return true;
        }

        bool RectMesh::create_border(const std::array<float, 2>& size, const GLfloat border_width,
                                     const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            priv::load_border_vertices(vertices_loc, size[0], size[1], border_width);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            // load ibo
            priv::load_border_ibo(m_ibo, m_ibo_length);

            return true;
        }

        void RectMesh::destroy() {}

    }
}
