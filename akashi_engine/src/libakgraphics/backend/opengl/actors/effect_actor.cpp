#include "./effect_actor.h"

#include "./layer_commons.h"
#include "../render_context.h"
#include "../camera.h"
#include "../fbo.h"
#include "../core/texture.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>

namespace akashi {
    namespace graphics {

        struct EffectActor::Pass : public layer_commons::CommonProgramLocation,
                                   public layer_commons::Transform {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;
        };

        bool EffectActor::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            if (m_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }
            m_pass = new EffectActor::Pass;
            CHECK_AK_ERROR2(this->load_pass(ctx));
            return true;
        }

        bool EffectActor::render(OGLRenderContext& ctx, const core::Rational& pts) {
            glUseProgram(m_pass->prog);

            ctx.fbo().resolve();
            if (OGLTexture fbo_tex; ctx.fbo().texture(fbo_tex)) {
                use_ogl_texture(fbo_tex, m_pass->tex_loc);
            }

            // clear underlying layers
            glEnable(GL_SCISSOR_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

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

        bool EffectActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                m_pass->mesh.destroy();
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;
            return true;
        }

        bool EffectActor::load_pass(const OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(layer_commons::load_shaders(m_pass->prog, m_layer_type,
                                                        m_layer_ctx.effect_layer_ctx.poly,
                                                        m_layer_ctx.effect_layer_ctx.frag));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture0");
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");

            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_pass->prog, "uvs");

            auto mesh_size = layer_commons::get_mesh_size(
                m_layer_ctx, {ctx.fbo().info().width, ctx.fbo().info().height});

            CHECK_AK_ERROR2(m_pass->mesh.create(mesh_size, vertices_loc, uvs_loc, true));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            // m_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.effect_layer_ctx.scale;
            layer_commons::update_model_mat(m_pass, m_layer_ctx);

            {
                glUseProgram(m_pass->prog);
                auto uv_flip_hv_loc = glGetUniformLocation(m_pass->prog, "uv_flip_hv");
                glUniform2i(uv_flip_hv_loc, m_layer_ctx.uv_flip_h, m_layer_ctx.uv_flip_v);
                glUseProgram(0);
            }

            return true;
        }

    }

}
