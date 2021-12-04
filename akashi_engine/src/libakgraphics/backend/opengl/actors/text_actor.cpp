#include "./text_actor.h"

#include "./layer_commons.h"
#include "../render_context.h"
#include "../camera.h"
#include "../resource/font.h"
#include "../fbo.h"
#include "../core/texture.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/element.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        struct TextActor::Pass : public layer_commons::CommonProgramLocation,
                                 public layer_commons::Transform {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;
            OGLTexture tex;
        };

        bool TextActor::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            if (m_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }
            m_pass = new TextActor::Pass;
            CHECK_AK_ERROR2(this->load_pass(ctx));
            return true;
        }

        bool TextActor::render(OGLRenderContext& ctx, const core::Rational& pts) {
            glUseProgram(m_pass->prog);

            use_ogl_texture(m_pass->tex, m_pass->tex_loc);

            glm::mat4 new_mvp = ctx.camera()->vp_mat() * m_pass->model_mat;

            glUniformMatrix4fv(m_pass->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            auto local_pts = pts - m_layer_ctx.from;
            glUniform1f(m_pass->time_loc, local_pts.to_decimal());
            glUniform1f(m_pass->global_time_loc, pts.to_decimal());

            auto local_duration = m_layer_ctx.to - m_layer_ctx.from;
            glUniform1f(m_pass->local_duration_loc, local_duration.to_decimal());
            glUniform1f(m_pass->fps_loc, ctx.fps().to_decimal());

            auto res = ctx.resolution();
            glUniform2f(m_pass->resolution_loc, res[0], res[1]);

            glBindVertexArray(m_pass->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pass->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_pass->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        bool TextActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                m_pass->mesh.destroy();
                free_ogl_texture(m_pass->tex);
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;

            return true;
        }

        bool TextActor::load_pass(OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(layer_commons::load_shaders(m_pass->prog, m_layer_ctx, m_layer_type));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            if (m_layer_type == core::LayerType::IMAGE) {
                m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture_arr");
            } else {
                m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture0");
            }
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");

            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_pass->prog, "uvs");

            CHECK_AK_ERROR2(this->load_texture(ctx));

            CHECK_AK_ERROR2(m_pass->mesh.create(
                {(float)m_pass->tex.effective_width, (float)m_pass->tex.effective_height},
                vertices_loc, uvs_loc));

            m_pass->trans_vec = layer_commons::get_trans_vec(m_layer_ctx);
            m_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.text_layer_ctx.scale;
            this->update_model_mat();

            return true;
        }

        bool TextActor::load_texture(OGLRenderContext& ctx) {
            SDL_Surface* surface = nullptr;

            FontInfo info;
            info.text = m_layer_ctx.text_layer_ctx.text;

            auto style = m_layer_ctx.text_layer_ctx.style;

            auto fg_color = hex_to_sdl(style.fill);
            info.fg = fg_color;

            auto font_path = style.font_path;
            info.font_path = font_path.empty() ? ctx.default_font_path() : font_path;

            auto font_size = style.font_size;
            int default_font_size = 30;
            info.font_size = font_size <= 0 ? default_font_size : font_size;

            if (FontLoader::GetInstance().getSurface(surface, info) != core::ErrorType::OK) {
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

        void TextActor::update_model_mat() {
            m_pass->model_mat = glm::translate(m_pass->model_mat, m_pass->trans_vec);
            m_pass->model_mat = glm::scale(m_pass->model_mat, m_pass->scale_vec);
        }

    }

}
