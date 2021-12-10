#include "./rect.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

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

        /* Rect */

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

        namespace priv {

            // Expects a suitable vao to be binded before calling this function
            static bool load_round_mesh(const GLuint vertices_loc, const GLfloat width,
                                        const GLfloat height, const GLfloat radius, GLuint& ibo,
                                        size_t& ibo_length) {
                auto hwidth = width * 0.5f;
                auto hheight = height * 0.5f;

                if (!(radius > 0)) {
                    AKLOG_ERRORN("radius must be larger than 0");
                    return false;
                }
                // [TODO] impl cases like radius == hwidth or radius == hheight later
                if (!(radius < std::min(hwidth, hheight))) {
                    AKLOG_ERRORN("radius is too large");
                    return false;
                }

                // [TODO] calc lod for different radius
                constexpr int lod = 64;
                std::array<GLfloat, 3> center_pos = {0.0, 0.0, 0.0};
                constexpr int vertice_point_length = (lod * 4) + 1;
                std::array<GLfloat, vertice_point_length * 3> vertices{0};

                auto delta = M_PI_2 / (lod - 1);

                const std::vector<std::array<GLfloat, 3>> centers = {
                    {-hwidth + radius, hheight - radius, 0}, // left-top
                    {hwidth - radius, hheight - radius, 0},  // right-top
                    {hwidth - radius, -hheight + radius, 0}, // right-bottom
                    {-hwidth + radius, -hheight + radius, 0} // left-bottom
                };

                int v_index = 0;
                vertices[v_index++] = center_pos[0];
                vertices[v_index++] = center_pos[1];
                vertices[v_index++] = center_pos[2];

                std::vector<std::array<GLfloat, 2>> buffers(lod, {0, 0});
                for (int i = 0; i < lod; i++) {
                    auto rad = delta * i;
                    buffers[i] = {(float)cos(rad) * radius, (float)sin(rad) * radius};
                }

                // left-top
                for (int i = 0; i < lod; i++) {
                    vertices[v_index++] = centers[0][0] - buffers[i][0]; // x
                    vertices[v_index++] = centers[0][1] + buffers[i][1]; // y
                    vertices[v_index++] = centers[0][2];                 // z
                }

                // right-top
                for (int i = 0; i < lod; i++) {
                    vertices[v_index++] = centers[1][0] + buffers[lod - i - 1][0]; // x
                    vertices[v_index++] = centers[1][1] + buffers[lod - i - 1][1]; // y
                    vertices[v_index++] = centers[1][2];                           // z
                }

                // right-bottom
                for (int i = 0; i < lod; i++) {
                    vertices[v_index++] = centers[2][0] + buffers[i][0]; // x
                    vertices[v_index++] = centers[2][1] - buffers[i][1]; // y
                    vertices[v_index++] = centers[2][2];                 // z
                }

                // left-bottom
                for (int i = 0; i < lod; i++) {
                    vertices[v_index++] = centers[3][0] - buffers[lod - i - 1][0]; // x
                    vertices[v_index++] = centers[3][1] - buffers[lod - i - 1][1]; // y
                    vertices[v_index++] = centers[3][2];                           // z
                }

                GLuint vertices_vbo;
                create_buffer(vertices_vbo, GL_ARRAY_BUFFER, vertices.data(), sizeof(vertices));

                glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
                glEnableVertexAttribArray(vertices_loc);
                glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                      (GLvoid*)0);

                constexpr size_t ibo_size = (4 * lod) * 3;
                auto ibo_index = 0;
                std::array<unsigned short, ibo_size> indices{0};

                for (size_t i = 1; i < vertice_point_length; i++) {
                    if (i == vertice_point_length - 1) {
                        indices[ibo_index++] = 1;
                        indices[ibo_index++] = i;
                        indices[ibo_index++] = 0;
                    } else {
                        indices[ibo_index++] = i + 1;
                        indices[ibo_index++] = i;
                        indices[ibo_index++] = 0;
                    }
                }

                create_buffer(ibo, GL_ELEMENT_ARRAY_BUFFER, indices.data(), sizeof(indices));
                ibo_length = ibo_size;

                return true;
            }

        }

        /* RoundRect */

        bool RoundRectMesh::create(const std::array<float, 2>& size, const GLfloat radius,
                                   const GLuint vertices_loc) {
            // load vao
            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);

            CHECK_AK_ERROR2(
                priv::load_round_mesh(vertices_loc, size[0], size[1], radius, m_ibo, m_ibo_length));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

        bool RoundRectMesh::create_border(const std::array<float, 2>& size,
                                          const GLfloat border_width, const GLuint vertices_loc) {
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

        void RoundRectMesh::destroy() {}

    }
}
