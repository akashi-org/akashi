#include "./unit_actor.h"

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

        struct UnitActor::Pass : public layer_commons::CommonProgramLocation,
                                 public layer_commons::Transform {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;

            core::borrowed_ptr<FBO> fbo{nullptr};
        };

        bool UnitActor::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            if (m_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }
            m_pass = new UnitActor::Pass;
            CHECK_AK_ERROR2(this->load_pass(ctx));
            return true;
        }

        bool UnitActor::render(OGLRenderContext& ctx, const core::Rational& pts,
                               const Camera& camera) {
            if (!m_pass->fbo) {
                AKLOG_ERRORN("FBO is null");
                return false;
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(m_pass->prog);

            if (OGLTexture fbo_tex; m_pass->fbo->texture(fbo_tex)) {
                use_ogl_texture(fbo_tex, m_pass->tex_loc);
            }

            glm::mat4 new_mvp = camera.vp_mat() * m_pass->model_mat;

            glUniformMatrix4fv(m_pass->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            auto local_pts = pts - m_layer_ctx.from;
            glUniform1f(m_pass->time_loc, local_pts.to_decimal());
            glUniform1f(m_pass->global_time_loc, pts.to_decimal());

            auto local_duration = m_layer_ctx.to - m_layer_ctx.from;
            glUniform1f(m_pass->local_duration_loc, local_duration.to_decimal());
            glUniform1f(m_pass->fps_loc, ctx.fps().to_decimal());

            auto res = m_layer_ctx.unit_layer_ctx.fb_size;
            glUniform2f(m_pass->resolution_loc, res[0], res[1]);

            glBindVertexArray(m_pass->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pass->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_pass->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            glEnable(GL_BLEND);
            ctx.use_default_blend_func();

            return true;
        }

        bool UnitActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                m_pass->mesh.destroy();
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;
            return true;
        }

        void UnitActor::set_fbo(const core::borrowed_ptr<FBO>& fbo_ptr) { m_pass->fbo = fbo_ptr; }

        bool UnitActor::load_pass(const OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(layer_commons::load_shaders(m_pass->prog, m_layer_type,
                                                        m_layer_ctx.unit_layer_ctx.poly,
                                                        m_layer_ctx.unit_layer_ctx.frag));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture0");
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");
            m_pass->mesh_size_loc = glGetUniformLocation(m_pass->prog, "mesh_size");

            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_pass->prog, "uvs");

            auto mesh_size = layer_commons::get_mesh_size(
                m_layer_ctx, {m_layer_ctx.layer_size[0], m_layer_ctx.layer_size[1]});

            CHECK_AK_ERROR2(m_pass->mesh.create(mesh_size, vertices_loc, uvs_loc, true));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            // m_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.effect_layer_ctx.scale;
            layer_commons::update_model_mat(m_pass, m_layer_ctx);

            {
                glUseProgram(m_pass->prog);
                auto uv_flip_hv_loc = glGetUniformLocation(m_pass->prog, "uv_flip_hv");
                glUniform2i(uv_flip_hv_loc, m_layer_ctx.uv_flip_h, m_layer_ctx.uv_flip_v);
                glUniform2fv(m_pass->mesh_size_loc, 1, m_pass->mesh.mesh_size().data());
                glUseProgram(0);
            }

            return true;
        }

    }

}
