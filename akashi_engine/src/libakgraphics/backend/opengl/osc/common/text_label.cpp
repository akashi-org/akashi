#include "./text_label.h"
#include "../osc_render_context.h"
#include "../osc_camera.h"

#include "../../core/glc.h"
#include "../../core/shader.h"
#include "../../core/texture.h"
#include "../../meshes/quad.h"
#include "../../resource/font.h"

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
    in vec2 uvs;

    out vec2 vUvs;
    
    void main(void){
        vUvs = uvs;
        gl_Position = u_mvp * vec4(vertices, 1.0);
})";

static constexpr const char* fshader_src = u8R"(
    #version 420 core
    uniform sampler2D texture0;
    in vec2 vUvs;

    out vec4 fragColor;

    void main(void){
        vec4 smpColor = texture(texture0, vUvs);
        fragColor = smpColor;
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

        struct TextLabel::Context {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;
            GLuint mvp_loc;
            OGLTexture tex;
            glm::mat4 model_mat = glm::mat4(1.0f);
        };

        TextLabel::TextLabel(const TextLabel::Params& init_params) : m_obj_params(init_params) {
            m_ctx = new TextLabel::Context;

            this->load_pass();
        }

        TextLabel::~TextLabel() {
            if (m_ctx) {
                m_ctx->mesh.destroy();
                free_ogl_texture(m_ctx->tex);
                glDeleteProgram(m_ctx->prog);
                delete m_ctx;
            }
            m_ctx = nullptr;
        }

        void TextLabel::update_obj_params(const TextLabel::Params& obj_params) {
            m_obj_params = obj_params;
            m_dirty = true;
        }

        void TextLabel::update_transform(const glm::mat4& model_mat) {
            m_ctx->model_mat = model_mat;
        }

        bool TextLabel::render(OSCRenderContext& render_ctx, const RenderParams& /*params*/) {
            if (m_dirty) {
                free_ogl_texture(m_ctx->tex);
                this->load_texture();
                // [TODO]  translate?
                m_dirty = false;
            }
            glUseProgram(m_ctx->prog);

            use_ogl_texture(m_ctx->tex, m_ctx->tex_loc);

            glm::mat4 new_mvp = render_ctx.camera()->vp_mat() * m_ctx->model_mat;

            glUniformMatrix4fv(m_ctx->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            glBindVertexArray(m_ctx->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ctx->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_ctx->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        bool TextLabel::load_pass() {
            m_ctx->prog = glCreateProgram();

            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(link_shader(m_ctx->prog));

            m_ctx->mvp_loc = glGetUniformLocation(m_ctx->prog, "u_mvp");
            m_ctx->tex_loc = glGetUniformLocation(m_ctx->prog, "texture0");

            CHECK_AK_ERROR2(this->load_texture());
            CHECK_AK_ERROR2(this->load_mesh());

            m_ctx->model_mat = glm::translate(m_ctx->model_mat,
                                              get_trans_vec({m_obj_params.cx, m_obj_params.cy, 0}));

            return true;
        }

        bool TextLabel::load_mesh() {
            auto vertices_loc = glGetAttribLocation(m_ctx->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_ctx->prog, "uvs");

            std::array<float, 2> mesh_size = {(float)m_obj_params.w, (float)m_obj_params.h};
            CHECK_AK_ERROR2(m_ctx->mesh.create(mesh_size, vertices_loc, uvs_loc));

            return true;
        }

        bool TextLabel::create_ogl_texture(OGLTexture& tex) {
            SDL_Surface* surface = nullptr;

            FontInfo info;
            info.text = m_obj_params.text;
            info.text_align = m_obj_params.text_align;
            info.pad = m_obj_params.pad;
            info.line_span = m_obj_params.line_span;

            auto style = m_obj_params.style;
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

            tex.image = surface->pixels;
            tex.width = surface->w;
            tex.height = surface->h;
            tex.effective_width = tex.width;
            tex.effective_height = tex.height;
            tex.format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            tex.surface = surface;

            return true;
        }

        bool TextLabel::load_texture() {
            CHECK_AK_ERROR2(this->create_ogl_texture(m_ctx->tex));

            glGenTextures(1, &m_ctx->tex.buffer);

            glBindTexture(GL_TEXTURE_2D, m_ctx->tex.buffer);
            glTexImage2D(GL_TEXTURE_2D, 0, m_ctx->tex.internal_format, m_ctx->tex.width,
                         m_ctx->tex.height, 0, m_ctx->tex.format, GL_UNSIGNED_BYTE,
                         m_ctx->tex.image);

            // glGenerateMipmap(GL_TEXTURE_2D);

            // [XXX] make sure to explicity setup when not using mimap
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);

            // SDL_FreeSurface((SDL_Surface*)surface);

            return true;
        }

    }
}
