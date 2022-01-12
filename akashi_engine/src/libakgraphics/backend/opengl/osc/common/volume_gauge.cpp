#include "./volume_gauge.h"
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
    uniform int u_warn;

    out vec4 fragColor;

    void main(void){
        fragColor = u_warn == 1 ? vec4(1,0,0,1) : u_color;
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

        struct VolumeGaugePass {
            GLuint prog;
            BaseMesh* mesh = nullptr;
            GLuint mvp_loc;
            GLuint warn_loc;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        struct VolumeGauge::Context {
            VolumeGaugePass border_pass;
            VolumeGaugePass content_pass;
        };

        VolumeGauge::VolumeGauge(const VolumeGauge::Params& init_params)
            : m_obj_params(init_params) {
            m_ctx = new VolumeGauge::Context;

            // ensure border_width >= 1.0
            m_obj_params.border_width = std::max(1.0, m_obj_params.border_width);

            this->load_border_pass();
            this->load_content_pass();
        }

        VolumeGauge::~VolumeGauge() {
            if (m_ctx) {
                m_ctx->border_pass.mesh->destroy();
                delete m_ctx->border_pass.mesh;
                glDeleteProgram(m_ctx->border_pass.prog);

                m_ctx->content_pass.mesh->destroy();
                delete m_ctx->content_pass.mesh;
                glDeleteProgram(m_ctx->content_pass.prog);

                delete m_ctx;
            }
            m_ctx = nullptr;
        }

        void VolumeGauge::update_obj_params(const VolumeGauge::Params& obj_params) {
            m_obj_params = obj_params;
        }

        bool VolumeGauge::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            CHECK_AK_ERROR2(this->render_content(render_ctx, params));
            CHECK_AK_ERROR2(this->render_border(render_ctx, params));
            return true;
        }

        bool VolumeGauge::load_border_pass() {
            auto& pass = m_ctx->border_pass;

            pass.prog = glCreateProgram();

            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(link_shader(pass.prog));

            pass.mvp_loc = glGetUniformLocation(pass.prog, "u_mvp");
            pass.warn_loc = glGetUniformLocation(pass.prog, "u_warn");

            // load mesh
            auto vertices_loc = glGetAttribLocation(pass.prog, "vertices");
            std::array<float, 2> mesh_size = {m_obj_params.w - (float)m_obj_params.border_width * 2,
                                              m_obj_params.h -
                                                  (float)m_obj_params.border_width * 2};

            pass.mesh = new RectMesh;
            CHECK_AK_ERROR2(static_cast<RectMesh*>(pass.mesh)->create_border(
                mesh_size, m_obj_params.border_width, vertices_loc));

            pass.model_mat = glm::translate(pass.model_mat,
                                            get_trans_vec({m_obj_params.cx, m_obj_params.cy, 0}));

            auto color_loc = glGetUniformLocation(pass.prog, "u_color");
            auto color = to_rgba_float(m_obj_params.border_color);

            glUseProgram(pass.prog);
            glUniform4fv(color_loc, 1, color.data());
            glUniform1i(pass.warn_loc, 0);
            glUseProgram(0);

            return true;
        }

        bool VolumeGauge::render_border(OSCRenderContext& render_ctx, const RenderParams& params) {
            auto pass = m_ctx->border_pass;

            glUseProgram(pass.prog);

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * pass.model_mat;

            glUniformMatrix4fv(pass.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(pass.mesh->vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass.mesh->ibo());

            glDrawElements(GL_TRIANGLES, pass.mesh->ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        bool VolumeGauge::load_content_pass() {
            auto& pass = m_ctx->content_pass;

            pass.prog = glCreateProgram();

            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(link_shader(pass.prog));

            pass.mvp_loc = glGetUniformLocation(pass.prog, "u_mvp");
            pass.warn_loc = glGetUniformLocation(pass.prog, "u_warn");

            // load mesh
            auto vertices_loc = glGetAttribLocation(pass.prog, "vertices");
            std::array<float, 2> mesh_size = {(float)m_obj_params.w, (float)m_obj_params.h};

            pass.mesh = new RectMesh;
            CHECK_AK_ERROR2(static_cast<RectMesh*>(pass.mesh)->create(mesh_size, vertices_loc));

            this->update_content_model_mat();

            auto color_loc = glGetUniformLocation(pass.prog, "u_color");
            auto color = to_rgba_float(m_obj_params.color);

            glUseProgram(pass.prog);
            glUniform4fv(color_loc, 1, color.data());
            glUseProgram(0);

            return true;
        }

        bool VolumeGauge::render_content(OSCRenderContext& render_ctx, const RenderParams& params) {
            auto& pass = m_ctx->content_pass;

            glUseProgram(pass.prog);

            this->update_content_model_mat();

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * pass.model_mat;

            glUniformMatrix4fv(pass.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glUniform1i(pass.warn_loc, m_obj_params.value > 1.0 ? 1 : 0);

            glBindVertexArray(pass.mesh->vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass.mesh->ibo());

            glDrawElements(GL_TRIANGLES, pass.mesh->ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        void VolumeGauge::update_content_model_mat() {
            auto& pass = m_ctx->content_pass;
            pass.model_mat = glm::mat4(1.0f);
            pass.model_mat = glm::translate(pass.model_mat,
                                            get_trans_vec({m_obj_params.cx, m_obj_params.cy, 0}));

            auto gain = std::min(1.0, m_obj_params.value);
            pass.model_mat =
                glm::translate(pass.model_mat, {-m_obj_params.w * 0.5 * (1 - gain), 0, 0});
            pass.model_mat = glm::scale(pass.model_mat, glm::vec3(gain, 1.0, 1.0));
        }

    }
}
