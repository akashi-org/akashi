#include "./layer_object.h"

#include "./layer_commons.h"
#include "./layer_shaders.h"
#include "./layer_buffers.h"
#include "./layer_video_texture.h"
#include "../meshes/quad.h"
#include "../core/texture.h"
#include "../core/shader.h"

#include "../render_context.h"
#include "../camera.h"
#include "../fbo.h"

#include "../core/texture.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakbuffer/avbuffer.h>

#include <SDL.h>
#include "../resource/image.h"

using namespace akashi::graphics::layer;

namespace akashi {
    namespace graphics {

        static void parse_safe_layer_ctx(SafeLayerContext* safe_ctx,
                                         const core::LayerContext& layer_ctx) {
            safe_ctx->uuid = layer_ctx.uuid;
            safe_ctx->atom_uuid = layer_ctx.atom_uuid;
            safe_ctx->key = layer_ctx.key;
            safe_ctx->display = layer_ctx.display;
            safe_ctx->from = layer_ctx.from;
            safe_ctx->layer_local_offset = layer_ctx.layer_local_offset;
            safe_ctx->to = layer_ctx.to;

            if (layer_ctx.t_transform) {
                safe_ctx->layer_size = layer_ctx.t_transform->layer_size;
            }
        }

        bool LayerObject::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx,
                                 const core::Rational& /*pts*/) {
            if (m_is_program_ready) {
                AKLOG_ERRORN("Pass already created");
                return false;
            }
            m_can_display = layer_ctx.display;
            m_layer_type = layer::get_layer_type(layer_ctx);
            parse_safe_layer_ctx(&m_safe_ctx, layer_ctx);
            if (m_layer_type == LayerType::UNIT) {
                m_unit_fb_size = layer_ctx.t_unit->fb_size;
            }

            {
                m_prog = glCreateProgram();

                CHECK_AK_ERROR2(this->load_shaders(layer_ctx));
                CHECK_AK_ERROR2(this->load_transform(layer_ctx));

                {
                    glUseProgram(m_prog);
                    // [XXX] There are unused textures in this layer.
                    // If we don't set the arbitrary value for those,
                    // GL_INVALID_OPERATION will occur in glDrawElements.
                    glUniform1i(UniformLocation::text_texture0, 10);
                    glUniform1i(UniformLocation::unit_texture0, 11);
                    glUniform1i(UniformLocation::shape_texture0, 12);
                    glUniform1i(UniformLocation::image_textures, 13);
                    glUniform1i(UniformLocation::video_textureY, 14);
                    glUniform1i(UniformLocation::video_textureCb, 15);
                    glUniform1i(UniformLocation::video_textureCr, 16);

                    if (layer_ctx.t_texture) {
                        glUniform2i(layer::UniformLocation::uv_flip_hv,
                                    layer_ctx.t_texture->uv_flip_h, layer_ctx.t_texture->uv_flip_v);
                    }

                    glUniform1f(layer::UniformLocation::main_tex_kind,
                                static_cast<float>(layer::get_layer_type(layer_ctx)));
                    glUniform1f(layer::UniformLocation::video_decode_method, 0);

                    glUseProgram(0);
                }

                if (m_layer_type != LayerType::VIDEO) {
                    CHECK_AK_ERROR2(layer::load_buffers(&m_texture, &m_mesh, ctx, layer_ctx));
                    this->set_buffers_ready();
                }

                m_is_program_ready = true;
            }
            return true;
        }

        bool LayerObject::update(OGLRenderContext& /*ctx*/, const core::LayerContext& layer_ctx,
                                 const core::Rational& /*pts*/) {
            m_can_display = layer_ctx.display;
            parse_safe_layer_ctx(&m_safe_ctx, layer_ctx);
            return true;
        }

        bool LayerObject::render(OGLRenderContext& ctx, const core::Rational& pts,
                                 const Camera& camera) {
            if (m_layer_type != LayerType::VIDEO && (!m_is_program_ready || !m_is_buffers_ready)) {
                AKLOG_WARNN("Not ready for rendering");
                return false;
            }
            if (m_layer_type == LayerType::UNIT && !m_fbo) {
                AKLOG_ERROR("FBO is null, {}, {}", m_safe_ctx.uuid.c_str(), m_safe_ctx.key.c_str());
                return false;
            }
            if (m_layer_type == LayerType::VIDEO) {
                if (m_current_pts == pts) {
                    AKLOG_INFON("PTS unchanged");
                    if (m_is_program_ready && m_is_buffers_ready) {
                        CHECK_AK_ERROR2(this->render_inner(ctx, pts, camera));
                        AKLOG_INFON("Rendering with the last frame");
                    }
                    return true;
                }

                auto buf_data = ctx.dequeue(m_safe_ctx.uuid, pts);
                if (!buf_data) {
                    AKLOG_INFON("Dequeue failed");
                    if (m_is_program_ready && m_is_buffers_ready) {
                        CHECK_AK_ERROR2(this->render_inner(ctx, pts, camera));
                        AKLOG_INFON("Rendering with the last frame");
                    }
                    return true;
                }

                if (!m_has_video_decode_method) {
                    {
                        glUseProgram(m_prog);
                        glUniform1f(layer::UniformLocation::video_decode_method,
                                    static_cast<float>(buf_data->prop().decode_method));
                        glUseProgram(0);
                    }
                    m_has_video_decode_method = true;
                }

                if (!m_is_buffers_ready) {
                    CHECK_AK_ERROR2(layer::load_video_buffers(&m_texture, &m_mesh, ctx, m_safe_ctx,
                                                              std::move(buf_data)));
                    this->set_buffers_ready();
                } else {
                    CHECK_AK_ERROR2(m_texture.video->update(ctx, std::move(buf_data)));
                }
            }

            CHECK_AK_ERROR2(this->render_inner(ctx, pts, camera));
            m_current_pts = pts;
            return true;
        }

        bool LayerObject::render_inner(OGLRenderContext& ctx, const core::Rational& pts,
                                       const Camera& camera) {
            bool has_premultiplied_alpha = m_layer_type == LayerType::TEXT or
                                           m_layer_type == LayerType::SHAPE or
                                           m_layer_type == LayerType::UNIT;
            {
                if (has_premultiplied_alpha) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                }

                glUseProgram(m_prog);

                if (m_layer_type == LayerType::UNIT) {
                    if (OGLTexture fbo_tex; m_fbo->texture(fbo_tex)) {
                        use_ogl_texture(fbo_tex, UniformLocation::unit_texture0);
                    }
                } else if (m_layer_type == LayerType::VIDEO) {
                    m_texture.video->use_textures({UniformLocation::video_textureY,
                                                   UniformLocation::video_textureCb,
                                                   UniformLocation::video_textureCr});
                } else {
                    use_ogl_texture(*m_texture.basic, m_texture.main_tex_loc);
                }

                glm::mat4 new_mvp = camera.vp_mat() * m_transform.model_mat;

                glUniformMatrix4fv(UniformLocation::mvpMatrix, 1, GL_FALSE, &new_mvp[0][0]);

                auto local_pts = (pts - m_safe_ctx.from) + m_safe_ctx.layer_local_offset;
                glUniform1f(UniformLocation::time, local_pts.to_decimal());
                glUniform1f(UniformLocation::global_time, pts.to_decimal());

                auto local_duration = m_safe_ctx.to - m_safe_ctx.from;
                glUniform1f(UniformLocation::location_duration, local_duration.to_decimal());
                glUniform1f(UniformLocation::fps, ctx.fps().to_decimal());

                auto res = m_layer_type == LayerType::UNIT ? m_unit_fb_size : ctx.resolution();
                glUniform2f(UniformLocation::resolution, res[0], res[1]);

                glBindVertexArray(m_mesh.quad->vao());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh.quad->ibo());

                glDrawElements(GL_TRIANGLES, m_mesh.quad->ibo_length(), GL_UNSIGNED_SHORT, 0);

                glBindVertexArray(0);

                if (has_premultiplied_alpha) {
                    glEnable(GL_BLEND);
                    ctx.use_default_blend_func();
                }
            }

            return true;
        }

        bool LayerObject::destroy(const OGLRenderContext& /*ctx*/) {
            if (m_mesh.quad) {
                m_mesh.quad->destroy();
                delete m_mesh.quad;
            }
            if (m_texture.basic) {
                free_ogl_texture(*m_texture.basic);
                delete m_texture.basic;
            }
            if (m_texture.video) {
                m_texture.video->destroy();
                delete m_texture.video;
            }
            m_texture.main_tex_loc = -1;

            glDeleteProgram(m_prog);
            return true;
        }

        void LayerObject::set_fbo(const core::borrowed_ptr<FBO>& fbo_ptr) { m_fbo = fbo_ptr; }

        void LayerObject::set_buffers_ready() {
            m_is_buffers_ready = true;
            {
                glUseProgram(m_prog);
                glUniform2fv(layer::UniformLocation::mesh_size, 1, m_mesh.quad->mesh_size().data());
                glUseProgram(0);
            }
        }

        bool LayerObject::load_shaders(const core::LayerContext& layer_ctx) {
            CHECK_AK_ERROR2(
                compile_attach_shader(m_prog, GL_VERTEX_SHADER, layer::vshader_src.c_str()));
            CHECK_AK_ERROR2(
                compile_attach_shader(m_prog, GL_FRAGMENT_SHADER, layer::fshader_src.c_str()));

            bool has_u_frag_shader =
                bool(layer_ctx.t_shader) && not(layer_ctx.t_shader->frag.empty());
            const std::string& frag_shader =
                has_u_frag_shader ? layer_ctx.t_shader->frag : layer::default_user_fshader_src;
            CHECK_AK_ERROR2(compile_attach_shader(m_prog, GL_FRAGMENT_SHADER, frag_shader.c_str()));

            bool has_u_poly_shader =
                bool(layer_ctx.t_shader) && not(layer_ctx.t_shader->poly.empty());
            const std::string& poly_shader =
                has_u_poly_shader ? layer_ctx.t_shader->poly : layer::default_user_pshader_src;
            CHECK_AK_ERROR2(compile_attach_shader(m_prog, GL_VERTEX_SHADER, poly_shader.c_str()));

            CHECK_AK_ERROR2(compile_attach_shader(m_prog, GL_GEOMETRY_SHADER,
                                                  layer::default_user_gshader_src.c_str()));

            CHECK_AK_ERROR2(link_shader(m_prog));

            return true;
        }

        static glm::vec3 get_trans_vec(const std::array<double, 3>& layer_pos) {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int screen_width = viewport[2];
            int screen_height = viewport[3];

            auto c_x = core::Rational(screen_width, 2);
            auto c_y = core::Rational(screen_height, 2);

            auto a_x = core::Rational(layer_pos[0]); // mouse coord
            auto a_y = core::Rational(layer_pos[1]); // mouse coord

            return glm::vec3((a_x - c_x).to_decimal(), -(a_y - c_y).to_decimal(), layer_pos[2]);
        }

        static void update_model_mat(layer::Transform* transform) {
            transform->model_mat = glm::translate(transform->model_mat, transform->trans_vec);
            transform->model_mat = glm::rotate(transform->model_mat, transform->rotation_rad,
                                               glm::vec3(0.0f, 0.0f, 1.0f));
            transform->model_mat = glm::scale(transform->model_mat, transform->scale_vec);
        }

        bool LayerObject::load_transform(const core::LayerContext& layer_ctx) {
            core::TransformTField transform_field;
            if (!layer_ctx.t_transform) {
                AKLOG_WARNN("TransformField not found");
            } else {
                transform_field = *layer_ctx.t_transform;
            }
            m_transform.trans_vec =
                get_trans_vec({transform_field.x, transform_field.y, transform_field.z});
            m_transform.scale_vec = glm::vec3(
                {transform_field.scale[0], transform_field.scale[1], transform_field.scale[2]});
            m_transform.rotation_rad = glm::radians((float)transform_field.rotation.to_decimal());

            update_model_mat(&m_transform);

            return true;
        }
    }

}
