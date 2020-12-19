#include "./framebuffer.h"

#include "./gl.h"
#include "./core/texture.h"
#include "./core/mvp.h"
#include "./objects/quad/quad.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        bool FramebufferObject::create(const GLRenderContext& ctx, int fbo_width, int fbo_height) {
            QuadMesh mesh;
            mesh.flip_y = -1;
            CHECK_AK_ERROR2(this->load_fbo(ctx, m_prop, mesh.tex, fbo_width, fbo_height));

            // [TODO] nullptr check
            m_prop.quad.create(ctx, *ctx.pass, std::move(mesh));

            return true;
        }

        bool FramebufferObject::render(const GLRenderContext& ctx) const {
            const auto& prop = m_prop.quad.get_prop();
            const auto& pass_prop = prop.pass.get_prop();
            const auto& mesh_prop = prop.mesh;

            GET_GLFUNC(ctx, glUseProgram)(pass_prop.prog);

            use_texture(ctx, mesh_prop.tex, pass_prop.tex_loc);

            glm::mat4 new_mvp = mesh_prop.mvp;
            update_scale(ctx, mesh_prop.tex, new_mvp);

            GET_GLFUNC(ctx, glUniformMatrix4fv)(pass_prop.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.flipY_loc, mesh_prop.flip_y);

            GET_GLFUNC(ctx, glBindVertexArray)(pass_prop.vao);
            // [XXX] required when using glDrawElements
            GET_GLFUNC(ctx, glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, pass_prop.ibo);

            GET_GLFUNC(ctx, glDrawElements)
            (GL_TRIANGLES, pass_prop.ibo_length, GL_UNSIGNED_SHORT, 0);

            GET_GLFUNC(ctx, glBindVertexArray)(0);

            return true;
        }

        bool FramebufferObject::destroy(const GLRenderContext& ctx) {
            GET_GLFUNC(ctx, glDeleteFramebuffers)(1, &m_prop.fbo);
            m_prop.quad.destroy(ctx);
            return true;
        }

        const FramebufferObject::Property& FramebufferObject::get_prop(void) const {
            return m_prop;
        }

        bool FramebufferObject::load_fbo(const GLRenderContext& ctx,
                                         FramebufferObject::Property& prop, GLTextureData& tex,
                                         int fbo_width, int fbo_height) const {
            prop.width = fbo_width;
            prop.height = fbo_height;

            GLint bounded_fbo;
            GET_GLFUNC(ctx, glGetIntegerv)(GL_FRAMEBUFFER_BINDING, &bounded_fbo);

            GET_GLFUNC(ctx, glGenFramebuffers)(1, &prop.fbo);
            GET_GLFUNC(ctx, glBindFramebuffer)(GL_FRAMEBUFFER, prop.fbo);

            // generate a texture for the fbo
            CHECK_AK_ERROR2(this->load_fbo_texture(ctx, tex, prop));

            GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
            GET_GLFUNC(ctx, glFramebufferTexture2D)
            (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.buffer, 0);

            GLenum err = GET_GLFUNC(ctx, glCheckFramebufferStatus)(GL_FRAMEBUFFER);

            GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, 0);
            GET_GLFUNC(ctx, glBindFramebuffer)(GL_FRAMEBUFFER, bounded_fbo);

            if (err != GL_FRAMEBUFFER_COMPLETE) {
                AKLOG_ERROR("FramebufferObject::load_fbo(): Failed to create framebufer (error={})",
                            err);
                return false;
            }
            return true;
        }

        bool FramebufferObject::load_fbo_texture(const GLRenderContext& ctx, GLTextureData& tex,
                                                 const FramebufferObject::Property& prop) const {
            tex.width = prop.width;
            tex.height = prop.height;
            tex.effective_width = tex.width;
            tex.effective_height = tex.height;
            tex.image = nullptr;
            tex.index = FramebufferObject::FBO_TEX_UNIT;

            // [TODO] error-check?
            create_texture(ctx, tex);

            return true;
        }

    }
}
