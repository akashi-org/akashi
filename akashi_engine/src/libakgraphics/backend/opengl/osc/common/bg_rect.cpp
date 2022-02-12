#include "./bg_rect.h"
#include "../osc_render_context.h"
#include "../osc_camera.h"

#include "../../core/glc.h"
#include "../../core/shader.h"
#include "../../core/color.h"
#include "../../meshes/rect.h"

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <libakcore/rational.h>

static constexpr const char* vshader_src = u8R"(
    #version 420 core
    uniform mat4 u_mvp;

    in vec3 vertices;
    
    void main(void){
        gl_Position = u_mvp * vec4(vertices, 1.0);
})";

static constexpr const char* fshader_src = u8R"(
    #version 420 core
    uniform vec4 u_color;

    out vec4 fragColor;

    void main(void){
        fragColor = u_color;
})";

namespace akashi {
    namespace graphics::osc {

        static glm::vec3 get_trans_vec(const std::array<int, 3>& layer_pos) {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];

            auto c_x = core::Rational(screen_width, 2);
            auto c_y = core::Rational(screen_height, 2);

            auto a_x = core::Rational(layer_pos[0], 1); // mouse coord
            auto a_y = core::Rational(layer_pos[1], 1); // mouse coord

            return glm::vec3((a_x - c_x).to_decimal(), -(a_y - c_y).to_decimal(), layer_pos[2]);
        }

        struct BGRect::Context {
            GLuint prog;
            RoundRectMesh mesh;
            GLuint mvp_loc;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        BGRect::BGRect(const BGRect::Params& init_params) : m_obj_params(init_params) {
            m_ctx = new BGRect::Context;

            this->load_pass();
        }

        BGRect::~BGRect() {
            if (m_ctx) {
                m_ctx->mesh.destroy();
                glDeleteProgram(m_ctx->prog);
                delete m_ctx;
            }
            m_ctx = nullptr;
        }

        void BGRect::update_obj_params(const BGRect::Params& obj_params) {
            m_obj_params = obj_params;
        }

        bool BGRect::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            glEnable(GL_MULTISAMPLE);

            glUseProgram(m_ctx->prog);

            // this->update_content_model_mat();

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * m_ctx->model_mat;

            glUniformMatrix4fv(m_ctx->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(m_ctx->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ctx->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_ctx->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            glDisable(GL_MULTISAMPLE);
            return true;
        }

        bool BGRect::load_pass() {
            m_ctx->prog = glCreateProgram();

            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(link_shader(m_ctx->prog));

            m_ctx->mvp_loc = glGetUniformLocation(m_ctx->prog, "u_mvp");

            // load mesh
            auto vertices_loc = glGetAttribLocation(m_ctx->prog, "vertices");
            std::array<float, 2> mesh_size = {(float)m_obj_params.w, (float)m_obj_params.h};

            CHECK_AK_ERROR2(m_ctx->mesh.create(mesh_size, m_obj_params.radius, vertices_loc));

            this->update_content_model_mat();

            auto color_loc = glGetUniformLocation(m_ctx->prog, "u_color");
            auto color = to_rgba_float(m_obj_params.color);

            glUseProgram(m_ctx->prog);
            glUniform4fv(color_loc, 1, color.data());
            glUseProgram(0);

            return true;
        }

        void BGRect::update_content_model_mat() {
            m_ctx->model_mat = glm::mat4(1.0f);
            m_ctx->model_mat = glm::translate(m_ctx->model_mat,
                                              get_trans_vec({m_obj_params.cx, m_obj_params.cy, 0}));
        }

    }
}
