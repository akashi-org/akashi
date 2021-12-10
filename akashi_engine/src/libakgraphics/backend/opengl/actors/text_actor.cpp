#include "./text_actor.h"

#include "./layer_commons.h"
#include "../render_context.h"
#include "../camera.h"
#include "../resource/font.h"
#include "../resource/image.h"
#include "../fbo.h"
#include "../core/texture.h"
#include "../meshes/rect.h"

#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakcore/element.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        struct TextActor::Pass : public layer_commons::CommonProgramLocation,
                                 public layer_commons::Transform {
            GLuint prog;
            BaseMesh* mesh = nullptr;
            GLuint tex_loc;
            OGLTexture tex;
        };

        bool TextActor::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            if (m_pass || m_lb_pass || m_border_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }
            m_pass = new TextActor::Pass;
            CHECK_AK_ERROR2(this->load_pass(ctx));

            m_lb_pass = new TextActor::Pass;
            CHECK_AK_ERROR2(this->load_label_pass(ctx));

            if (m_layer_ctx.text_layer_ctx.border.size > 0) {
                m_border_pass = new TextActor::Pass;
                CHECK_AK_ERROR2(this->load_border_pass(ctx));
            }

            return true;
        }

        bool TextActor::render(OGLRenderContext& ctx, const core::Rational& pts) {
            if (m_pass && m_lb_pass) {
                this->render_pass(*m_lb_pass, ctx, pts);
                if (m_border_pass) {
                    this->render_pass(*m_border_pass, ctx, pts);
                }
                this->render_pass(*m_pass, ctx, pts);
            }
            return true;
        }

        bool TextActor::destroy(const OGLRenderContext& /*ctx */) {
            auto passes = {m_pass, m_lb_pass, m_border_pass};
            for (auto&& pass : passes) {
                if (pass) {
                    if (pass->mesh) {
                        pass->mesh->destroy();
                        delete pass->mesh;
                    }
                    free_ogl_texture(pass->tex);
                    glDeleteProgram(pass->prog);
                    delete pass;
                }
            }

            m_pass = nullptr;
            m_lb_pass = nullptr;
            m_border_pass = nullptr;

            return true;
        }

        bool TextActor::load_pass(OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(layer_commons::load_shaders(m_pass->prog, m_layer_type,
                                                        m_layer_ctx.text_layer_ctx.poly,
                                                        m_layer_ctx.text_layer_ctx.frag));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture0");
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");

            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_pass->prog, "uvs");

            CHECK_AK_ERROR2(this->load_texture(ctx));

            m_pass->mesh = new QuadMesh;

            CHECK_AK_ERROR2(static_cast<QuadMesh*>(m_pass->mesh)
                                ->create({(float)m_pass->tex.effective_width,
                                          (float)m_pass->tex.effective_height},
                                         vertices_loc, uvs_loc));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            m_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.text_layer_ctx.scale;
            this->update_model_mat(*m_pass);

            return true;
        }

        namespace priv {

            static constexpr const char* default_label_user_fshader_src_head = u8R"(
    #version 420 core
    uniform float time;
    uniform float global_time;
    uniform float local_duration;
    uniform float fps;
    uniform vec2 resolution;
    void frag_main(inout vec4 _fragColor){ _fragColor = vec4)";

            static std::string to_color_glsl_str(const std::string& color_str) {
                auto sdl_color = hex_to_sdl(color_str);
                return "(" + std::to_string(sdl_color.b / 255.0) + "," +
                       std::to_string(sdl_color.g / 255.0) + "," +
                       std::to_string(sdl_color.r / 255.0) + "," +
                       std::to_string(sdl_color.a / 255.0) + ")";
            }

            static std::string default_label_user_fshader_src(const std::string& color_str) {
                return default_label_user_fshader_src_head + to_color_glsl_str(color_str) + ";}";
            }
        }

        bool TextActor::load_label_pass(OGLRenderContext& /*ctx*/) {
            assert(this->m_pass);

            m_lb_pass->prog = glCreateProgram();

            auto label_src = m_layer_ctx.text_layer_ctx.label.src;

            auto frag_shader = m_layer_ctx.text_layer_ctx.label.frag;
            if (frag_shader.empty() && label_src.empty()) {
                frag_shader =
                    priv::default_label_user_fshader_src(m_layer_ctx.text_layer_ctx.label.color);
            }

            CHECK_AK_ERROR2(layer_commons::load_shaders(
                m_lb_pass->prog, m_layer_type, m_layer_ctx.text_layer_ctx.label.poly, frag_shader));

            m_lb_pass->mvp_loc = glGetUniformLocation(m_lb_pass->prog, "mvpMatrix");
            m_lb_pass->tex_loc = glGetUniformLocation(m_lb_pass->prog, "texture0");
            m_lb_pass->time_loc = glGetUniformLocation(m_lb_pass->prog, "time");
            m_lb_pass->global_time_loc = glGetUniformLocation(m_lb_pass->prog, "global_time");
            m_lb_pass->local_duration_loc = glGetUniformLocation(m_lb_pass->prog, "local_duration");
            m_lb_pass->fps_loc = glGetUniformLocation(m_lb_pass->prog, "fps");
            m_lb_pass->resolution_loc = glGetUniformLocation(m_lb_pass->prog, "resolution");

            auto vertices_loc = glGetAttribLocation(m_lb_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_lb_pass->prog, "uvs");

            if (label_src.empty()) {
                glGenTextures(1, &m_lb_pass->tex.buffer);
            } else {
                this->load_label_texture(*m_lb_pass, label_src);
            }

            auto radius = m_layer_ctx.text_layer_ctx.label.radius;

            if (radius > 0) {
                m_lb_pass->mesh = new RoundRectMesh;
                static_cast<RoundRectMesh*>(m_lb_pass->mesh)
                    ->create(
                        {(float)m_pass->tex.effective_width, (float)m_pass->tex.effective_height},
                        radius, vertices_loc);
            } else {
                m_lb_pass->mesh = new QuadMesh;
                static_cast<QuadMesh*>(m_lb_pass->mesh)
                    ->create(
                        {(float)m_pass->tex.effective_width, (float)m_pass->tex.effective_height},
                        vertices_loc, uvs_loc);
            }

            m_lb_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            // [TODO] label pass should have a different scale value than text pass
            m_lb_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.text_layer_ctx.scale;
            this->update_model_mat(*m_lb_pass);

            return true;
        }

        bool TextActor::load_border_pass(OGLRenderContext& /*ctx*/) {
            assert(this->m_pass);

            m_border_pass->prog = glCreateProgram();

            auto border_color = m_layer_ctx.text_layer_ctx.border.color;
            auto border_width = m_layer_ctx.text_layer_ctx.border.size;
            auto border_radius = m_layer_ctx.text_layer_ctx.border.radius;

            auto frag_shader = priv::default_label_user_fshader_src(border_color);

            CHECK_AK_ERROR2(
                layer_commons::load_shaders(m_border_pass->prog, m_layer_type, "", frag_shader));

            m_border_pass->mvp_loc = glGetUniformLocation(m_border_pass->prog, "mvpMatrix");
            m_border_pass->tex_loc = glGetUniformLocation(m_border_pass->prog, "texture0");
            m_border_pass->time_loc = glGetUniformLocation(m_border_pass->prog, "time");
            m_border_pass->global_time_loc =
                glGetUniformLocation(m_border_pass->prog, "global_time");
            m_border_pass->local_duration_loc =
                glGetUniformLocation(m_border_pass->prog, "local_duration");
            m_border_pass->fps_loc = glGetUniformLocation(m_border_pass->prog, "fps");
            m_border_pass->resolution_loc = glGetUniformLocation(m_border_pass->prog, "resolution");

            auto vertices_loc = glGetAttribLocation(m_border_pass->prog, "vertices");

            glGenTextures(1, &m_border_pass->tex.buffer);

            m_border_pass->mesh = new RectMesh;

            CHECK_AK_ERROR2(static_cast<RectMesh*>(m_border_pass->mesh)
                                ->create_border({(float)m_pass->tex.effective_width,
                                                 (float)m_pass->tex.effective_height},
                                                border_width, vertices_loc));

            m_border_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            // [TODO] border pass should have a different scale value than text pass ?
            m_border_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.text_layer_ctx.scale;
            this->update_model_mat(*m_border_pass);

            return true;
        }

        bool TextActor::load_texture(OGLRenderContext& ctx) {
            SDL_Surface* surface = nullptr;

            FontInfo info;
            info.text = m_layer_ctx.text_layer_ctx.text;
            info.text_align = m_layer_ctx.text_layer_ctx.text_align;
            info.pad = m_layer_ctx.text_layer_ctx.pad;
            info.line_span = m_layer_ctx.text_layer_ctx.line_span;

            auto style = m_layer_ctx.text_layer_ctx.style;
            info.color = hex_to_sdl(style.fg_color);
            auto font_path = style.font_path;
            info.font_path = font_path.empty() ? ctx.default_font_path() : font_path;
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

            m_pass->tex.image = surface->pixels;
            m_pass->tex.width = surface->w;
            m_pass->tex.height = surface->h;
            m_pass->tex.effective_width = m_pass->tex.width;
            m_pass->tex.effective_height = m_pass->tex.height;
            m_pass->tex.format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            m_pass->tex.surface = surface;

            glGenTextures(1, &m_pass->tex.buffer);

            glBindTexture(GL_TEXTURE_2D, m_pass->tex.buffer);
            glTexImage2D(GL_TEXTURE_2D, 0, m_pass->tex.internal_format, m_pass->tex.width,
                         m_pass->tex.height, 0, m_pass->tex.format, GL_UNSIGNED_BYTE,
                         m_pass->tex.image);

            // glGenerateMipmap(GL_TEXTURE_2D);

            // [XXX] make sure to explicity setup when not using mimap
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);

            return true;
        }

        bool TextActor::load_label_texture(TextActor::Pass& pass, const std::string& src) {
            if (src.empty()) {
                AKLOG_ERRORN("Failed to getSurface");
                return false;
            }
            SDL_Surface* surface = nullptr;

            if (ImageLoader::GetInstance().getSurface(surface, src.c_str()) !=
                core::ErrorType::OK) {
                AKLOG_ERROR("Failed to getSurface: {}", src.c_str());
                return false;
            }

            pass.tex.width = surface->w;
            pass.tex.height = surface->h;
            pass.tex.effective_width = surface->w;  // !!
            pass.tex.effective_height = surface->h; // !!
            pass.tex.format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            pass.tex.internal_format = GL_RGBA8;
            pass.tex.target = GL_TEXTURE_2D;
            pass.tex.surface = surface;
            pass.tex.image = surface->pixels;

            glGenTextures(1, &pass.tex.buffer);
            glBindTexture(GL_TEXTURE_2D, pass.tex.buffer);

            glTexImage2D(GL_TEXTURE_2D, 0, pass.tex.internal_format, pass.tex.width,
                         pass.tex.height, 0, pass.tex.format, GL_UNSIGNED_BYTE, pass.tex.image);

            // GET_GLFUNC(ctx, glGenerateMipmap)(GL_TEXTURE_2D_ARRAY);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);

            // SDL_FreeSurface(surface);

            return true;
        }

        bool TextActor::render_pass(const TextActor::Pass& pass, OGLRenderContext& ctx,
                                    const core::Rational& pts) {
            glUseProgram(pass.prog);

            use_ogl_texture(pass.tex, pass.tex_loc);

            glm::mat4 new_mvp = ctx.camera()->vp_mat() * pass.model_mat;

            glUniformMatrix4fv(pass.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            auto local_pts = pts - m_layer_ctx.from;
            glUniform1f(pass.time_loc, local_pts.to_decimal());
            glUniform1f(pass.global_time_loc, pts.to_decimal());

            auto local_duration = m_layer_ctx.to - m_layer_ctx.from;
            glUniform1f(pass.local_duration_loc, local_duration.to_decimal());
            glUniform1f(pass.fps_loc, ctx.fps().to_decimal());

            auto res = ctx.resolution();
            glUniform2f(pass.resolution_loc, res[0], res[1]);

            glBindVertexArray(pass.mesh->vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pass.mesh->ibo());

            glDrawElements(GL_TRIANGLES, pass.mesh->ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        void TextActor::update_model_mat(TextActor::Pass& pass) {
            pass.model_mat = glm::translate(pass.model_mat, pass.trans_vec);
            pass.model_mat = glm::scale(pass.model_mat, pass.scale_vec);
        }

    }

}
