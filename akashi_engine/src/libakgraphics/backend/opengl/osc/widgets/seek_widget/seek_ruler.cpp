#include "./seek_ruler.h"
#include "../../osc_render_context.h"
#include "../../osc_camera.h"

#include "../../../core/glc.h"
#include "../../../core/shader.h"
#include "../../../core/color.h"
#include "../../../meshes/line.h"
#include "../../../core/texture.h"
#include "../../../meshes/quad.h"
#include "../../../resource/font.h"

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>
#include <libakcore/time.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <libakcore/rational.h>

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

        struct SeekLinePass {
            GLuint prog;
            LineMesh mesh;
            GLuint mvp_loc;
            glm::mat4 model_mat = glm::mat4(1.0f);
            GLuint ruler_width_loc;
        };

        struct SeekLabelPass {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;
            GLuint mvp_loc;
            GLuint label_offset_loc;
            OGLTexture tex;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        struct SeekRuler::Context {
            SeekLinePass line_pass;

            float line_size;
            std::string line_color;
            std::array<long, 2> line_begin;
            std::array<long, 2> line_end;

            SeekLabelPass label_pass;
        };

        SeekRuler::SeekRuler(OSCRenderContext& render_ctx, const BoundingBox& bbox,
                             const SeekRuler::Params& init_params)
            : BaseWidget(render_ctx, bbox), m_obj_params(init_params) {
            m_ctx = new SeekRuler::Context;

            m_ctx->line_size = 1.0;
            m_ctx->line_color = "#ffffff";
            m_ctx->line_begin = {m_bbox.cx, m_calc_bbox.top};
            m_ctx->line_end = {m_bbox.cx, m_calc_bbox.bottom};

            this->load_line_pass();
            this->load_label_pass(render_ctx);
        }

        SeekRuler::~SeekRuler() {
            if (m_ctx) {
                m_ctx->line_pass.mesh.destroy();
                glDeleteProgram(m_ctx->line_pass.prog);

                m_ctx->label_pass.mesh.destroy();
                glDeleteTextures(1, &m_ctx->label_pass.tex.buffer);
                glDeleteProgram(m_ctx->label_pass.prog);
                delete m_ctx;
            }
            m_ctx = nullptr;
        }

        void SeekRuler::update_obj_params(const SeekRuler::Params& obj_params) {
            for (size_t i = 0; i < m_obj_params.label_texts.size(); i++) {
                if (m_obj_params.label_texts[i] != obj_params.label_texts[i]) {
                    m_label_dirty = true;
                }
            }
            m_obj_params = obj_params;
        }

        bool SeekRuler::update(OSCRenderContext& render_ctx, const RenderParams& params) {
            if (m_label_dirty) {
                m_ctx->label_pass.mesh.destroy();
                glDeleteTextures(1, &m_ctx->label_pass.tex.buffer);

                this->load_texture(render_ctx);
                this->load_label_mesh();

                m_label_dirty = false;
                return true;
            }

            return false;
        }

        bool SeekRuler::render(OSCRenderContext& render_ctx, const RenderParams& params) {
            CHECK_AK_ERROR2(this->render_line_pass(render_ctx, params));
            CHECK_AK_ERROR2(this->render_label_pass(render_ctx, params));
            return true;
        }

        bool SeekRuler::load_line_pass() {
            auto& pass = m_ctx->line_pass;

            pass.prog = glCreateProgram();

            // clang-format off
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_VERTEX_SHADER,
                u8R"(
                    #version 420 core
                    uniform mat4 u_mvp;
                    uniform float u_ruler_width;
                    uniform float u_ruler_height;
                
                    in vec3 vertices;

                    float offset_x(float base_offset) {
                        float inst_offset = u_ruler_width * gl_InstanceID;
                        return inst_offset + (gl_InstanceID % 10 != 0 ? 0.0 :
                               gl_VertexID % 2 == 0 ? -base_offset :
                               base_offset);
                    }

                    float offset_y(float base_offset) {
                        return gl_InstanceID % 10 != 0 ? 0.0 :
                               gl_VertexID <= 1 ? base_offset :
                               0.0;
                    }

                    void main(void){
                        gl_Position = u_mvp * vec4(
                            vertices.x + offset_x(0.0) + (2.0), 
                            vertices.y + offset_y(u_ruler_height * 0.6),
                            vertices.z, 
                            1.0
                        );
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
            pass.ruler_width_loc = glGetUniformLocation(pass.prog, "u_ruler_width");

            auto vertices_loc = glGetAttribLocation(pass.prog, "vertices");
            CHECK_AK_ERROR2(pass.mesh.create_default(vertices_loc, m_ctx->line_size,
                                                     m_ctx->line_begin, m_ctx->line_end));

            glBindVertexArray(pass.mesh.vao());
            glVertexAttribDivisor(vertices_loc, 0);
            glBindVertexArray(0);

            pass.model_mat = glm::translate(pass.model_mat, get_trans_vec({0, 0, 0}));

            auto color_loc = glGetUniformLocation(pass.prog, "u_color");
            auto color = to_rgba_float(m_ctx->line_color);

            auto ruler_height_loc = glGetUniformLocation(pass.prog, "u_ruler_height");

            glUseProgram(pass.prog);
            glUniform4fv(color_loc, 1, color.data());
            glUniform1f(pass.ruler_width_loc,
                        ((1.0 * m_obj_params.seek_area_width) / m_obj_params.seek_ruler));
            glUniform1f(ruler_height_loc, m_ctx->line_end[1] - m_ctx->line_begin[1]);
            glUseProgram(0);

            return true;
        }

        bool SeekRuler::render_line_pass(OSCRenderContext& render_ctx, const RenderParams& params) {
            auto& pass = m_ctx->line_pass;

            glUseProgram(pass.prog);

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * pass.model_mat;

            glUniformMatrix4fv(pass.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(pass.mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass.mesh.ibo());

            glDrawElementsInstanced(GL_TRIANGLES, pass.mesh.ibo_length(), GL_UNSIGNED_SHORT,
                                    nullptr, m_obj_params.seek_ruler);

            glBindVertexArray(0);

            return true;
        }

        bool SeekRuler::load_label_pass(OSCRenderContext& render_ctx) {
            auto& pass = m_ctx->label_pass;

            pass.prog = glCreateProgram();

            // clang-format off
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_VERTEX_SHADER,
                u8R"(
                    #version 420 core
                    uniform mat4 u_mvp;
                    uniform float u_label_offset;
                
                    in vec3 vertices;
                    in vec2 uvs;

                    out vec2 vUvs;
                    out float vo_tex_idx;

                    void main(void){
                        vUvs = uvs;
                        vo_tex_idx = gl_InstanceID;
                        gl_Position = u_mvp * vec4(
                            vertices.x + (u_label_offset * gl_InstanceID),
                            vertices.y,
                            vertices.z, 
                            1.0
                        );
                })"
            ));
            // clang-format on

            // clang-format off
            CHECK_AK_ERROR2(compile_attach_shader(pass.prog, GL_FRAGMENT_SHADER,
                u8R"(
                    #version 420 core
                    uniform sampler2DArray u_texture;

                    in vec2 vUvs;
                    in float vo_tex_idx;

                    out vec4 fragColor;
                
                    void main(void){
                        vec4 smpColor = texture(u_texture, vec3(vUvs, vo_tex_idx));
                        fragColor = smpColor;
                        // fragColor.r = fragColor.a < 0.1 ? 1.0 : fragColor.r;
                        // fragColor.a = fragColor.a < 0.1 ? 1.0 : fragColor.a;
                })"
            ));
            // clang-format on

            CHECK_AK_ERROR2(link_shader(pass.prog));

            pass.mvp_loc = glGetUniformLocation(pass.prog, "u_mvp");
            pass.label_offset_loc = glGetUniformLocation(pass.prog, "u_label_offset");
            pass.tex_loc = glGetUniformLocation(pass.prog, "u_texture");

            CHECK_AK_ERROR2(this->load_texture(render_ctx));
            CHECK_AK_ERROR2(this->load_label_mesh());

            glUseProgram(pass.prog);
            glUniform1f(pass.label_offset_loc,
                        ((1.0 * m_obj_params.seek_area_width) / (m_obj_params.label_texts.size())));
            glUseProgram(0);

            return true;
        }

        bool SeekRuler::load_texture(OSCRenderContext& render_ctx) {
            auto& pass = m_ctx->label_pass;

            std::vector<SDL_Surface*> surfaces(m_obj_params.label_texts.size(), nullptr);

            int max_tex_width = 0;
            int max_tex_height = 0;
            for (size_t i = 0; i < surfaces.size(); i++) {
                if (!this->create_font_surface(
                        surfaces[i], this->format_label(render_ctx, m_obj_params.label_texts[i]),
                        m_obj_params.label)) {
                    AKLOG_ERROR("Failed to getSurface: {}", m_obj_params.label_texts[i]);
                    return false;
                }

                max_tex_width = std::max(surfaces[i]->w, max_tex_width);
                max_tex_height = std::max(surfaces[i]->h, max_tex_height);
            }

            pass.tex.width = max_tex_width;
            pass.tex.height = max_tex_height;
            pass.tex.effective_width = max_tex_width;
            pass.tex.effective_height = max_tex_height;
            pass.tex.format = (surfaces[0]->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            pass.tex.internal_format =
                (surfaces[0]->format->BytesPerPixel == 3) ? GL_RGB8 : GL_RGBA8;
            pass.tex.target = GL_TEXTURE_2D_ARRAY;

            glGenTextures(1, &pass.tex.buffer);

            glBindTexture(GL_TEXTURE_2D_ARRAY, pass.tex.buffer);

            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, pass.tex.internal_format, max_tex_width,
                           max_tex_height, surfaces.size());

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Hacks for handling texture array with different sizes for each textures
            // ref: https://gamedev.stackexchange.com/a/121220
            std::vector<uint32_t> clear_data(max_tex_width * max_tex_height, 0);
            for (size_t i = 0; i < surfaces.size(); i++) {
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, max_tex_width, max_tex_height, 1,
                                pass.tex.format, GL_UNSIGNED_BYTE, &clear_data[0]);

                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, surfaces[i]->w, surfaces[i]->h, 1,
                                pass.tex.format, GL_UNSIGNED_BYTE, surfaces[i]->pixels);
            }

            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

            for (auto&& surface : surfaces) {
                SDL_FreeSurface(surface);
            }

            return true;
        }

        std::string SeekRuler::format_label(OSCRenderContext& render_ctx, long frame_num) {
            // second mode
            if (render_ctx.is_second_mode()) {
                auto sec_per_frame = core::Rational(1l) / render_ctx.fps();
                auto frame_msec = (core::Rational(1000 * frame_num, 1) * sec_per_frame);
                return core::to_time_string(frame_msec.to_decimal(), false);
            }
            // frame mode
            else {
                return std::to_string(frame_num);
            }
        }

        bool SeekRuler::create_font_surface(SDL_Surface*& surface, const std::string& text,
                                            const SeekRuler::LabelParams& label_params) const {
            FontInfo info;

            info.text = text; // use `text` from arugments not the one from label_params

            info.text_align = label_params.text_align;
            info.pad = label_params.pad;
            info.line_span = label_params.line_span;

            auto style = label_params.style;
            info.color = hex_to_sdl(style.fg_color);
            auto font_path = style.font_path;
            // info.font_path = font_path.empty() ? ctx.default_font_path() : font_path;
            info.font_path = font_path;
            info.size = style.fg_size <= 0 ? 0 : style.fg_size;

            FontOutline outline;
            outline.size = style.outline_size;
            outline.color = hex_to_sdl(style.outline_color);

            FontShadow shadow;
            shadow.color = hex_to_sdl(style.shadow_color);
            shadow.size = style.shadow_size;

            if (!FontLoader::GetInstance().get_surface(surface, info,
                                                       style.use_outline ? &outline : nullptr,
                                                       style.use_shadow ? &shadow : nullptr)) {
                AKLOG_ERRORN("Failed to getFontsSurface");
                return false;
            }

            return true;
        }

        bool SeekRuler::load_label_mesh() {
            auto& pass = m_ctx->label_pass;

            auto vertices_loc = glGetAttribLocation(pass.prog, "vertices");
            auto uvs_loc = glGetAttribLocation(pass.prog, "uvs");

            auto mesh_height = (m_ctx->line_end[1] - m_ctx->line_begin[1]) * 2.2;
            auto aspect_ratio = core::Rational(pass.tex.effective_width, 1) /
                                core::Rational(pass.tex.effective_height, 1);
            auto mesh_width = core::Rational(mesh_height, 1) * aspect_ratio;

            std::array<float, 2> mesh_size = {(float)mesh_width.to_decimal(), (float)mesh_height};

            CHECK_AK_ERROR2(pass.mesh.create(mesh_size, vertices_loc, uvs_loc));

            glBindVertexArray(pass.mesh.vao());
            glVertexAttribDivisor(vertices_loc, 0);
            glBindVertexArray(0);

            pass.model_mat = glm::mat4(1.0f);
            pass.model_mat = glm::translate(
                pass.model_mat, get_trans_vec({int(m_bbox.cx + (mesh_size[0] * 0.5)),
                                               int(m_bbox.cy - (mesh_size[1] * 1.1)), 0}));

            return true;
        }

        bool SeekRuler::render_label_pass(OSCRenderContext& render_ctx,
                                          const RenderParams& params) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            auto& pass = m_ctx->label_pass;

            glUseProgram(pass.prog);

            use_ogl_texture(pass.tex, pass.tex_loc);

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * pass.model_mat;

            glUniformMatrix4fv(pass.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(pass.mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass.mesh.ibo());

            glDrawElementsInstanced(GL_TRIANGLES, pass.mesh.ibo_length(), GL_UNSIGNED_SHORT,
                                    nullptr, m_obj_params.label_texts.size());

            glBindVertexArray(0);

            render_ctx.use_default_blend_func();

            return true;
        }

    }
}
