#include "./shape_actor.h"

#include "./layer_commons.h"
#include "../render_context.h"
#include "../camera.h"
#include "../fbo.h"
#include "../core/texture.h"
#include "../resource/font.h"

#include "../meshes/rect.h"
#include "../meshes/circle.h"
#include "../meshes/triangle.h"
#include "../meshes/line.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>

#include <libakvgfx/akvgfx.h>
#include <libakvgfx/item.h>

namespace akashi {
    namespace graphics {

        struct ShapeActor::Pass : public layer_commons::CommonProgramLocation,
                                  public layer_commons::Transform {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;
            OGLTexture tex;
        };

        bool ShapeActor::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            if (m_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }
            m_pass = new ShapeActor::Pass;
            CHECK_AK_ERROR2(this->load_pass(ctx));
            return true;
        }

        bool ShapeActor::render(OGLRenderContext& ctx, const core::Rational& pts,
                                const Camera& camera) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(m_pass->prog);

            use_ogl_texture(m_pass->tex, m_pass->tex_loc);

            glm::mat4 new_mvp = camera.vp_mat() * m_pass->model_mat;

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

            glEnable(GL_BLEND);
            ctx.use_default_blend_func();

            return true;
        }

        bool ShapeActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                m_pass->mesh.destroy();
                glDeleteTextures(1, &m_pass->tex.buffer);
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;
            return true;
        }

        bool ShapeActor::load_pass(OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(layer_commons::load_shaders(m_pass->prog, m_layer_type,
                                                        m_layer_ctx.shape_layer_ctx.poly,
                                                        m_layer_ctx.shape_layer_ctx.frag));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");
            m_pass->mesh_size_loc = glGetUniformLocation(m_pass->prog, "mesh_size");

            m_pass->tex_loc = glGetUniformLocation(m_pass->prog, "texture0");

            CHECK_AK_ERROR2(this->load_mesh(ctx));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
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

        bool ShapeActor::load_mesh(OGLRenderContext& /* ctx */) {
            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto uvs_loc = glGetAttribLocation(m_pass->prog, "uvs");

            auto shape_params = m_layer_ctx.shape_layer_ctx;
            auto shape_kind = shape_params.shape_kind;
            switch (shape_kind) {
                case core::ShapeKind::RECT: {
                    std::array<float, 2> mesh_size = {(float)shape_params.rect.width,
                                                      (float)shape_params.rect.height};

                    const int border_padv = 4;
                    const float border_pad = shape_params.border_size * border_padv;

                    CHECK_AK_ERROR2(
                        m_pass->mesh.create({mesh_size[0] + border_pad, mesh_size[1] + border_pad},
                                            vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = border_padv},
                                                        shape_params);
                    CHECK_AK_ERROR2(this->load_texture(*surface));
                    break;
                }

                case core::ShapeKind::CIRCLE: {
                    std::array<float, 2> mesh_size = {(float)m_layer_ctx.layer_size[0],
                                                      (float)m_layer_ctx.layer_size[1]};

                    const int border_padv = 4;
                    const float border_pad = shape_params.border_size * border_padv;

                    CHECK_AK_ERROR2(
                        m_pass->mesh.create({mesh_size[0] + border_pad, mesh_size[1] + border_pad},
                                            vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = border_padv},
                                                        shape_params);
                    CHECK_AK_ERROR2(this->load_texture(*surface));

                    break;
                }

                case core::ShapeKind::TRIANGLE: {
                    float mesh_width = shape_params.tri.width;
                    if (shape_params.tri.wr < 0 || shape_params.tri.wr > 1) {
                        mesh_width += (2 * std::abs(shape_params.tri.wr) * shape_params.tri.width);
                    }
                    float mesh_height = shape_params.tri.height;

                    const int border_padv = 4;
                    const float border_pad = shape_params.border_size * border_padv;

                    std::array<float, 2> mesh_size = {mesh_width, mesh_height};

                    CHECK_AK_ERROR2(
                        m_pass->mesh.create({mesh_width + border_pad, mesh_height + border_pad},
                                            vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = border_padv},
                                                        shape_params);
                    CHECK_AK_ERROR2(this->load_texture(*surface));
                    break;
                }
                case core::ShapeKind::LINE: {
                    std::array<float, 2> mesh_size = {(float)m_layer_ctx.layer_size[0],
                                                      (float)m_layer_ctx.layer_size[1]};

                    CHECK_AK_ERROR2(m_pass->mesh.create(mesh_size, vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = 0},
                                                        shape_params);
                    CHECK_AK_ERROR2(this->load_texture(*surface));

                    break;
                }

                default: {
                    AKLOG_ERROR("Invalid or not implemented shape kind {} found", shape_kind);
                    break;
                }
            }

            return true;
        }

        bool ShapeActor::load_texture(const vgfx::Surface& surface) {
            m_pass->tex.image = surface.buffer();
            m_pass->tex.width = surface.info().width;
            m_pass->tex.height = surface.info().height;
            m_pass->tex.effective_width = m_pass->tex.width;
            m_pass->tex.effective_height = m_pass->tex.height;

            m_pass->tex.format = surface.info().format == vgfx::SurfaceFormat::RGB24
                                     ? (surface.info().format_swap ? GL_BGR : GL_RGB)
                                     : (surface.info().format_swap ? GL_BGRA : GL_RGBA);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            {
                glGenTextures(1, &m_pass->tex.buffer);

                glBindTexture(GL_TEXTURE_2D, m_pass->tex.buffer);

                int bytes_per_pixel = surface.info().format == vgfx::SurfaceFormat::RGB24 ? 3 : 4;
                glPixelStorei(GL_UNPACK_ROW_LENGTH, surface.info().stride / bytes_per_pixel);
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, m_pass->tex.internal_format, m_pass->tex.width,
                                 m_pass->tex.height, 0, m_pass->tex.format, GL_UNSIGNED_BYTE,
                                 m_pass->tex.image);
                }
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

                // [XXX] make sure to explicity setup when not using mimap
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glBindTexture(GL_TEXTURE_2D, 0);
            }
            glPixelStorei(GL_UNPACK_ALIGNMENT, graphics::DEFAULT_UNPACK_ALIGNMENT);

            return true;
        }

    }
}
