#include "./fbo.h"
#include "./meshes/quad.h"
#include "./core/shader.h"
#include "./core/texture.h"
#include "./render_context.h"
#include "./camera.h"

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakcore/rational.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

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
    namespace graphics {

        struct FBO::Pass {
            GLuint prog;

            QuadMesh mesh;

            GLuint tex_loc;

            GLuint mvp_loc;

            OGLTexture tex;

            OGLTexture msaa_tex;

            GLuint dst_fbo;

            GLuint depth_buffer;
        };

        bool FBO::create(int fbo_width, int fbo_height, int msaa) {
            if (m_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }

            m_pass = new FBO::Pass;

            m_info.width = fbo_width;
            m_info.height = fbo_height;

            CHECK_AK_ERROR2(this->load_msaa_texture(msaa));
            CHECK_AK_ERROR2(this->load_texture());
            CHECK_AK_ERROR2(this->load_pass());

            CHECK_AK_ERROR2(this->load_fbo(msaa));

            m_initialized = true;

            return true;
        }

        bool FBO::render(OGLRenderContext& ctx) {
            if (!m_initialized) {
                AKLOG_ERRORN("FBO is not yet initialized");
                return false;
            }

            glUseProgram(m_pass->prog);

            use_ogl_texture(m_pass->tex, m_pass->tex_loc);

            glm::mat4 new_mvp = ctx.camera()->vp_mat() * this->get_model_mat();

            glUniformMatrix4fv(m_pass->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);
            glBindVertexArray(m_pass->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pass->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_pass->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        void FBO::resolve() const {
            GLint bounded_fbo;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bounded_fbo);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_info.fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pass->dst_fbo);
            glBlitFramebuffer(0, 0, m_info.width, m_info.height, 0, 0, m_info.width, m_info.height,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);

            glBindFramebuffer(GL_FRAMEBUFFER, bounded_fbo);
        }

        void FBO::destroy() {
            if (m_pass) {
                glDeleteRenderbuffers(1, &m_pass->depth_buffer);
                m_pass->mesh.destroy();
                free_ogl_texture(m_pass->tex);
                free_ogl_texture(m_pass->msaa_tex);
                glDeleteProgram(m_pass->prog);
                glDeleteFramebuffers(1, &m_pass->dst_fbo);
                delete m_pass;
            }
            glDeleteFramebuffers(1, &m_info.fbo);
            m_pass = nullptr;
            m_initialized = false;
        }

        const FBInfo& FBO::info() const { return m_info; }

        bool FBO::dst_fbo(GLuint* fbo) const {
            if (m_pass) {
                *fbo = m_pass->dst_fbo;
                return true;
            } else {
                return false;
            }
        }

        bool FBO::texture(OGLTexture& in_tex) const {
            if (m_pass) {
                in_tex = m_pass->tex;
                return true;
            } else {
                return false;
            }
        }

        bool FBO::msaa_texture(OGLTexture& in_tex) const {
            if (m_pass) {
                in_tex = m_pass->msaa_tex;
                return true;
            } else {
                return false;
            }
        }

        bool FBO::load_pass() {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(compile_attach_shader(m_pass->prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(m_pass->prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(link_shader(m_pass->prog));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "u_mvp");
            m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture0");

            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_pass->prog, "uvs");

            CHECK_AK_ERROR2(m_pass->mesh.create({(float)m_info.width, (float)m_info.height},
                                                vertices_loc, uvs_loc, true));

            return true;
        }

        bool FBO::load_texture() {
            m_pass->tex.width = m_info.width;
            m_pass->tex.height = m_info.height;
            m_pass->tex.effective_width = m_info.width;
            m_pass->tex.effective_height = m_info.height;
            m_pass->tex.image = nullptr;
            m_pass->tex.index = FBO::FBO_TEX_UNIT;
            m_pass->tex.target = GL_TEXTURE_2D;

            glGenTextures(1, &m_pass->tex.buffer);

            glBindTexture(GL_TEXTURE_2D, m_pass->tex.buffer);
            glTexImage2D(GL_TEXTURE_2D, 0, m_pass->tex.internal_format, m_pass->tex.width,
                         m_pass->tex.height, 0, m_pass->tex.format, GL_UNSIGNED_BYTE,
                         m_pass->tex.image);

            // GET_GLFUNC(ctx, glGenerateMipmap)(GL_TEXTURE_2D);

            // [XXX] make sure to explicity setup when not using mimap
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);

            CHECK_GL_ERRORS();

            return true;
        }

        bool FBO::load_msaa_texture(int msaa) {
            m_pass->msaa_tex.width = m_info.width;
            m_pass->msaa_tex.height = m_info.height;
            m_pass->msaa_tex.effective_width = m_info.width;
            m_pass->msaa_tex.effective_height = m_info.height;
            m_pass->msaa_tex.image = nullptr;
            m_pass->msaa_tex.index = FBO::FBO_TEX_UNIT;
            m_pass->msaa_tex.target = GL_TEXTURE_2D_MULTISAMPLE;

            glGenTextures(1, &m_pass->msaa_tex.buffer);

            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pass->msaa_tex.buffer);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaa,
                                    m_pass->msaa_tex.internal_format, m_pass->msaa_tex.width,
                                    m_pass->msaa_tex.height, GL_TRUE);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

            CHECK_GL_ERRORS();

            return true;
        }

        bool FBO::load_fbo(int msaa) {
            GLint bounded_fbo;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bounded_fbo);

            glGenFramebuffers(1, &m_info.fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_info.fbo);

            glGenRenderbuffers(1, &m_pass->depth_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_pass->depth_buffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, GL_DEPTH_COMPONENT,
                                             m_info.width, m_info.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                      m_pass->depth_buffer);
            // [TODO] stencil?

            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pass->msaa_tex.buffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                                   m_pass->msaa_tex.buffer, 0);

            GLenum err = glCheckFramebufferStatus(GL_FRAMEBUFFER);

            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

            if (err != GL_FRAMEBUFFER_COMPLETE) {
                AKLOG_ERROR("Failed to create FBO: 0x{:x}, {}", err, gl_err_to_str(err));
                return false;
            }

            glGenFramebuffers(1, &m_pass->dst_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_pass->dst_fbo);

            glBindTexture(GL_TEXTURE_2D, m_pass->tex.buffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   m_pass->tex.buffer, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (err != GL_FRAMEBUFFER_COMPLETE) {
                AKLOG_ERROR("Failed to create FBO: 0x{:x}, {}", err, gl_err_to_str(err));
                return false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, bounded_fbo);

            return true;
        }

        glm::vec3 FBO::get_sar_scale_vec(const OGLTexture& tex) const {
            // adjustment of aspect ratio
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];
            core::Rational aspect =
                core::Rational(tex.effective_width, 1) / core::Rational(tex.effective_height, 1);
            core::Rational scale_w = core::Rational(1l);
            core::Rational scale_h = core::Rational(1l);

            if (tex.effective_width < screen_width && tex.height < screen_height) {
                scale_w = core::Rational(tex.effective_width, 1) / core::Rational(screen_width, 1);
                scale_h =
                    core::Rational(tex.effective_height, 1) / core::Rational(screen_height, 1);

            } else if (screen_width > screen_height) {
                // fixed height
                scale_w =
                    (core::Rational(screen_height, 1) * aspect) / core::Rational(screen_width, 1);
                scale_h = core::Rational(1l);
                if (scale_w > core::Rational(1l)) {
                    scale_h /= scale_w;
                    scale_w = core::Rational(1l);
                }
            } else {
                // fixed width
                scale_w = core::Rational(1l);
                scale_h =
                    (core::Rational(screen_width, 1) / aspect) / core::Rational(screen_height, 1);
                if (scale_h > core::Rational(1l)) {
                    scale_w /= scale_h;
                    scale_h = core::Rational(1l);
                }
            }

            return glm::vec3(scale_w.to_decimal(), scale_h.to_decimal(), 1.0);
        }

        glm::mat4 FBO::get_model_mat() const {
            glm::mat4 model_mat{1.0f};
            model_mat = glm::scale(model_mat, this->get_sar_scale_vec(m_pass->tex));
            return model_mat;
        }

    }
}
