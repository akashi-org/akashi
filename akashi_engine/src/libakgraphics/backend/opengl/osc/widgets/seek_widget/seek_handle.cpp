#include "./seek_handle.h"

#include "../../osc_render_context.h"
#include "../../../../../osc.h"

#include "../../osc_render_context.h"
#include "../../osc_camera.h"
#include "../../common/text_label.h"

#include "../../../core/glc.h"
#include "../../../core/shader.h"
#include "../../../meshes/line.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>
#include <libakcore/color.h>

namespace akashi {
    namespace graphics::osc {

        struct HandleLinePass {
            GLuint prog;
            LineMesh mesh;
            GLuint mvp_loc;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        struct HandleKnobPass {
            core::owned_ptr<osc::TextLabel> text_label = nullptr;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        struct SeekHandle::Context {
            HandleLinePass line_pass;
            float line_size;
            std::string line_color;
            std::array<long, 2> line_begin;
            std::array<long, 2> line_end;

            HandleKnobPass knob_pass;
            int knob_cx = 0;
            int knob_cy = 0;
        };

        SeekHandle::SeekHandle(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                               const SeekHandle::Params& params)
            : BaseWidget(render_ctx, bbox), m_obj_params(params) {
            m_ctx = new SeekHandle::Context;

            m_ctx->line_size = m_bbox.w;
            // m_ctx->line_color = "#ccf500"; // yellow
            // m_ctx->line_color = "#00ff55"; // green
            m_ctx->line_color = "#00dbac"; // neon light green
            m_ctx->line_begin = {m_bbox.cx, m_calc_bbox.top};
            m_ctx->line_end = {m_bbox.cx, m_calc_bbox.bottom};

            m_ctx->knob_cx = m_bbox.cx;
            m_ctx->knob_cy = m_calc_bbox.top * 0.98;

            this->load_line_pass();
            this->load_knob_pass(render_ctx);
        }

        SeekHandle::~SeekHandle() {
            if (m_ctx) {
                m_ctx->line_pass.mesh.destroy();
                glDeleteProgram(m_ctx->line_pass.prog);
                delete m_ctx;
            }
        }

        void SeekHandle::update_obj_params(const SeekHandle::Params& obj_params) {
            m_obj_params = obj_params;
        }

        bool SeekHandle::update(OSCRenderContext& /*render_ctx*/, const RenderParams& /*params*/) {
            this->update_line_transform(m_ctx->line_pass.model_mat);

            this->update_knob_transform(m_ctx->knob_pass.model_mat);
            m_ctx->knob_pass.text_label->update_transform(m_ctx->knob_pass.model_mat);
            return true;
        }

        bool SeekHandle::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            CHECK_AK_ERROR2(this->render_line_pass(render_ctx, params));
            CHECK_AK_ERROR2(this->render_knob_pass(render_ctx, params));
            return true;
        }

        bool SeekHandle::load_line_pass() {
            auto& pass = m_ctx->line_pass;

            pass.prog = glCreateProgram();

            // clang-format off
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_VERTEX_SHADER,
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
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_FRAGMENT_SHADER,
                u8R"(
                    #version 420 core
                    uniform vec4 u_color;
                
                    out vec4 fragColor;
                
                    void main(void){
                        fragColor = u_color;
                })"
            ));
            // clang-format on

            CHECK_AK_ERROR2(link_shader(pass.prog));

            pass.mvp_loc = glGetUniformLocation(pass.prog, "u_mvp");

            auto vertices_loc = glGetAttribLocation(pass.prog, "vertices");
            CHECK_AK_ERROR2(pass.mesh.create_default(vertices_loc, m_ctx->line_size,
                                                     m_ctx->line_begin, m_ctx->line_end));

            this->update_line_transform(pass.model_mat);

            auto color_loc = glGetUniformLocation(pass.prog, "u_color");
            auto color = core::to_rgba_float(m_ctx->line_color);

            glUseProgram(pass.prog);
            glUniform4fv(color_loc, 1, color.data());
            glUseProgram(0);

            return true;
        }

        bool SeekHandle::render_line_pass(OSCRenderContext& render_ctx,
                                          const RenderParams& params) {
            auto& pass = m_ctx->line_pass;

            glUseProgram(pass.prog);

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * pass.model_mat;

            glUniformMatrix4fv(pass.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(pass.mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass.mesh.ibo());

            glDrawElements(GL_TRIANGLES, pass.mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        bool SeekHandle::load_knob_pass(OSCRenderContext& render_ctx) {
            osc::TextLabel::Params text_params;
            text_params.text = "\uf0d7";
            text_params.style.font_path = render_ctx.default_font_path();
            if (std::getenv("AK_ASSET_DIR")) {
                text_params.style.font_path =
                    std::string(std::getenv("AK_ASSET_DIR")) +
                    "/fonts/fontawesome-free-5.12.1-desktop/otfs/Font Awesome 5 Free-Solid-900.otf";
            }

            text_params.style.fg_color = m_ctx->line_color;
            text_params.style.fg_size = 22;

            text_params.cx = m_ctx->knob_cx;
            text_params.cy = m_ctx->knob_cy;
            text_params.w = m_bbox.w * 6;
            text_params.h = m_bbox.w * 9;

            m_ctx->knob_pass.text_label = core::make_owned<osc::TextLabel>(text_params);

            this->update_knob_transform(m_ctx->knob_pass.model_mat);
            m_ctx->knob_pass.text_label->update_transform(m_ctx->knob_pass.model_mat);

            return true;
        }

        bool SeekHandle::render_knob_pass(OSCRenderContext& render_ctx,
                                          const RenderParams& params) {
            return m_ctx->knob_pass.text_label->render(render_ctx, params);
        }

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

        void SeekHandle::update_line_transform(glm::mat4& model_mat) {
            model_mat = glm::mat4(1.0f);
            model_mat = glm::translate(
                model_mat,
                get_trans_vec({int(m_obj_params.seek_area_width * m_obj_params.seek_value), 0, 0}));
        }

        void SeekHandle::update_knob_transform(glm::mat4& model_mat) {
            model_mat = glm::mat4(1.0f);
            model_mat = glm::translate(
                model_mat,
                get_trans_vec(
                    {int(m_obj_params.seek_area_width * m_obj_params.seek_value) + m_ctx->knob_cx,
                     m_ctx->knob_cy, 0}));
        }

    }
}
