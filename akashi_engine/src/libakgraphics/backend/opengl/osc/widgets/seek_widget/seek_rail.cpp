#include "./seek_rail.h"
#include "../../osc_render_context.h"
#include "../../osc_camera.h"

#include "../../../core/glc.h"
#include "../../../core/shader.h"
#include "../../../meshes/line.h"

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <libakcore/rational.h>
#include <libakcore/color.h>

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

        struct SeekRail::Context {
            GLuint prog;
            LineMesh mesh;
            GLuint mvp_loc;
            glm::mat4 model_mat = glm::mat4(1.0f);

            float line_size;
            std::string line_color;
            std::array<long, 2> line_begin;
            std::array<long, 2> line_end;
        };

        SeekRail::SeekRail(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                           const SeekRail::Params& init_params)
            : BaseWidget(render_ctx, bbox), m_obj_params(init_params) {
            m_ctx = new SeekRail::Context;

            m_ctx->line_size = 2;
            m_ctx->line_color = "#dddddd";
            m_ctx->line_begin = {m_calc_bbox.left, m_bbox.cy};
            m_ctx->line_end = {m_calc_bbox.right, m_bbox.cy};

            this->load_pass();
        }

        SeekRail::~SeekRail() {
            if (m_ctx) {
                m_ctx->mesh.destroy();
                glDeleteProgram(m_ctx->prog);
                delete m_ctx;
            }
            m_ctx = nullptr;
        }

        void SeekRail::update_obj_params(const SeekRail::Params& obj_params) {
            m_obj_params = obj_params;
        }

        bool SeekRail::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            return false;
        }

        bool SeekRail::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            glUseProgram(m_ctx->prog);

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * m_ctx->model_mat;

            glUniformMatrix4fv(m_ctx->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(m_ctx->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ctx->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_ctx->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        bool SeekRail::load_pass() {
            m_ctx->prog = glCreateProgram();

            // clang-format off
            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_VERTEX_SHADER,
                u8R"(
                    #version 420 core
                    uniform mat4 u_mvp;
                
                    in vec3 vertices;
                    
                    void main(void){
                        gl_Position = u_mvp * vec4(vertices, 1.0);
                })"
            ));
            // clang-format on

            // clang-format off
            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_FRAGMENT_SHADER,
                u8R"(
                    #version 420 core
                    uniform vec4 u_color;
                
                    out vec4 fragColor;
                
                    void main(void){
                        fragColor = u_color;
                })"
            ));
            // clang-format on

            CHECK_AK_ERROR2(link_shader(m_ctx->prog));

            m_ctx->mvp_loc = glGetUniformLocation(m_ctx->prog, "u_mvp");

            auto vertices_loc = glGetAttribLocation(m_ctx->prog, "vertices");
            CHECK_AK_ERROR2(m_ctx->mesh.create_default(vertices_loc, m_ctx->line_size,
                                                       m_ctx->line_begin, m_ctx->line_end));

            m_ctx->model_mat = glm::translate(m_ctx->model_mat, get_trans_vec({0, 0, 0}));

            auto color_loc = glGetUniformLocation(m_ctx->prog, "u_color");
            auto color = core::to_rgba_float(m_ctx->line_color);

            glUseProgram(m_ctx->prog);
            glUniform4fv(color_loc, 1, color.data());
            glUseProgram(0);

            return true;
        }

    }
}
