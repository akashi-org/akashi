#include "./grid.h"

#include "../core/glc.h"
#include "../core/shader.h"

#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <vector>

namespace akashi {
    namespace graphics {

        struct Grid::Pass {
            GLuint prog;
            GLuint vao;
            GLuint ibo;
            int ibo_length;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        Grid::Grid(Grid::State* state) {
            if (state) {
                m_state = *state;
            }

            m_pass = new Grid::Pass;
            m_pass->model_mat = glm::rotate(m_pass->model_mat, float(M_PI) / 2, glm::vec3(1, 0, 0));

            m_pass->prog = glCreateProgram();

            const char* axis_vhader = u8R"(
            #version 420 core
            #extension GL_ARB_explicit_uniform_location : enable
           
            layout(location = 0) in vec3 vertices;
            layout(location = 1) uniform uint line_counts;
            layout(location = 2) uniform mat4 mvpMatrix;

            // if 0, x-axis, if 1, z-axis, otherwise, invalid value
            layout(location = 3) uniform uint axis_type;

            flat out int is_main;
        
            void main(void){
                float sign = gl_VertexID == 0 ? -1: 1;
                float axis_width = line_counts;
                float axis_offset = gl_InstanceID - int(line_counts * 0.5);
                // axis_offset = axis_offset < 0 ? axis_offset : axis_offset + 1.0;

                vec3 pos = vec3(0,0,0);
                pos[0] = axis_type == 0 ? sign * axis_width : axis_offset;
                pos[2] = axis_type == 0 ? axis_offset : sign * axis_width;
                gl_Position = mvpMatrix * vec4(pos * vec3(1, 1, 1), 1.0);

                is_main = int(axis_offset) % 5 == 0 ? 1: -1;
            }
        )";

            compile_attach_shader(m_pass->prog, GL_VERTEX_SHADER, axis_vhader);

            const char* axis_fshader = u8R"(
            #version 420 core
            #extension GL_ARB_explicit_uniform_location : enable
            
            flat in int is_main;

            layout(location = 4) uniform vec4 u_base_color;
            layout(location = 5) uniform vec4 u_main_color;

            out vec4 fragColor;

            void main(void){
                fragColor = u_main_color;
            }
        )";

            compile_attach_shader(m_pass->prog, GL_FRAGMENT_SHADER, axis_fshader);
            link_shader(m_pass->prog);

            this->init_vertices();

            glUseProgram(m_pass->prog);
            glUniform1ui(1, m_state.line_counts);
            glUniform1ui(3, (uint32_t)m_state.type);
            glUniform4fv(4, 1, m_state.base_color.data());
            glUniform4fv(5, 1, m_state.main_color.data());
            glUseProgram(0);
        }

        Grid::~Grid() {
            if (m_pass) {
                glDeleteVertexArrays(1, &m_pass->vao);
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
        }

        void Grid::update() {}

        void Grid::render(const glm::mat4& pv) {
            glUseProgram(m_pass->prog);

            glm::mat4 new_mvp = pv * m_pass->model_mat;

            glUniformMatrix4fv(2, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(m_pass->vao);

            glDrawArraysInstanced(GL_LINES, 0, 2, m_state.line_counts);

            glBindVertexArray(0);
        }

        bool Grid::init_vertices() {
            // create buffer
            std::vector<GLfloat> vertices(m_state.line_counts * 6, 0);
            GLuint vertices_vbo;

            glGenBuffers(1, &vertices_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(),
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // create vao
            glGenVertexArrays(1, &m_pass->vao);
            glBindVertexArray(m_pass->vao);

            // register buffers to vao

            // vertices attribute
            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            GLint verticesLoc = glGetAttribLocation(m_pass->prog, "vertices");
            glEnableVertexAttribArray(verticesLoc);
            glVertexAttribPointer(verticesLoc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
            glVertexAttribDivisor(verticesLoc, 0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            return true;
        }

    }
}
