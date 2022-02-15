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

namespace akashi {
    namespace graphics {

        struct ShapeActor::Pass : public layer_commons::CommonProgramLocation,
                                  public layer_commons::Transform {
            GLuint prog;
            BaseMesh* mesh = nullptr;
            bool enable_msaa = false;
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

        bool ShapeActor::render(OGLRenderContext& ctx, const core::Rational& pts) {
            if (m_pass->enable_msaa) {
                glEnable(GL_MULTISAMPLE);
            }

            glUseProgram(m_pass->prog);

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

            glBindVertexArray(m_pass->mesh->vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pass->mesh->ibo());

            glDrawElements(GL_TRIANGLES, m_pass->mesh->ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            glDisable(GL_MULTISAMPLE);

            return true;
        }

        bool ShapeActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                if (m_pass->mesh) {
                    m_pass->mesh->destroy();
                }
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;
            return true;
        }

        namespace priv {

            static constexpr const char* default_shape_user_fshader_src_head = u8R"(
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

            static std::string default_shape_user_fshader_src(const std::string& color_str) {
                return default_shape_user_fshader_src_head + to_color_glsl_str(color_str) + ";}";
            }
        }

        bool ShapeActor::load_pass(const OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            auto shape_color = m_layer_ctx.shape_layer_ctx.color;
            auto frag_shader = m_layer_ctx.shape_layer_ctx.frag;
            if (frag_shader.empty() && !shape_color.empty()) {
                frag_shader = priv::default_shape_user_fshader_src(shape_color);
            }

            CHECK_AK_ERROR2(layer_commons::load_shaders(
                m_pass->prog, m_layer_type, m_layer_ctx.shape_layer_ctx.poly, frag_shader));

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");
            m_pass->mesh_size_loc = glGetUniformLocation(m_pass->prog, "mesh_size");

            CHECK_AK_ERROR2(this->load_mesh(ctx));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            layer_commons::update_model_mat(m_pass, m_layer_ctx);

            {
                glUseProgram(m_pass->prog);
                glUniform2fv(m_pass->mesh_size_loc, 1, m_pass->mesh->mesh_size().data());
                glUseProgram(0);
            }

            return true;
        }

        bool ShapeActor::load_mesh(const OGLRenderContext& /* ctx */) {
            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");

            auto shape_params = m_layer_ctx.shape_layer_ctx;
            auto shape_kind = shape_params.shape_kind;
            switch (shape_kind) {
                case core::ShapeKind::RECT: {
                    std::array<float, 2> mesh_size = {(float)shape_params.rect.width,
                                                      (float)shape_params.rect.height};
                    if (m_layer_ctx.layer_size[0] > 0) {
                        mesh_size[0] = m_layer_ctx.layer_size[0];
                    }
                    if (m_layer_ctx.layer_size[1] > 0) {
                        mesh_size[1] = m_layer_ctx.layer_size[1];
                    }

                    if (shape_params.edge_radius > 0) {
                        m_pass->mesh = new RoundRectMesh;
                        m_pass->enable_msaa = true;
                        if (shape_params.fill || !(shape_params.border_size > 0)) {
                            static_cast<RoundRectMesh*>(m_pass->mesh)
                                ->create(mesh_size, shape_params.edge_radius, vertices_loc);
                        } else {
                            static_cast<RoundRectMesh*>(m_pass->mesh)
                                ->create_border(
                                    {mesh_size[0] - (float)shape_params.border_size * 2,
                                     mesh_size[1] - (float)shape_params.border_size * 2},
                                    shape_params.edge_radius, shape_params.border_size,
                                    vertices_loc);
                        }
                    } else {
                        m_pass->mesh = new RectMesh;
                        if (shape_params.fill || !(shape_params.border_size > 0)) {
                            CHECK_AK_ERROR2(static_cast<RectMesh*>(m_pass->mesh)
                                                ->create(mesh_size, vertices_loc));
                        } else {
                            CHECK_AK_ERROR2(
                                static_cast<RectMesh*>(m_pass->mesh)
                                    ->create_border(
                                        {mesh_size[0] - (float)shape_params.border_size * 2,
                                         mesh_size[1] - (float)shape_params.border_size * 2},
                                        shape_params.border_size, vertices_loc));
                        }
                    }
                    break;
                }

                case core::ShapeKind::CIRCLE: {
                    m_pass->enable_msaa = true;

                    std::array<float, 2> mesh_size = {(float)shape_params.circle.radius,
                                                      (float)shape_params.circle.radius};
                    if (m_layer_ctx.layer_size[0] > 0) {
                        mesh_size[0] = m_layer_ctx.layer_size[0];
                    }
                    if (m_layer_ctx.layer_size[1] > 0) {
                        mesh_size[1] = m_layer_ctx.layer_size[1];
                    }

                    if (shape_params.fill || !(shape_params.border_size > 0)) {
                        m_pass->mesh = new CircleMesh;
                        CHECK_AK_ERROR2(
                            static_cast<CircleMesh*>(m_pass->mesh)
                                ->create(mesh_size, shape_params.circle.lod, vertices_loc));
                    } else {
                        m_pass->mesh = new CircleMesh;
                        CHECK_AK_ERROR2(
                            static_cast<CircleMesh*>(m_pass->mesh)
                                ->create_border(
                                    {mesh_size[0] - (float)shape_params.border_size * 2,
                                     mesh_size[1] - (float)shape_params.border_size * 2},
                                    shape_params.circle.lod, shape_params.border_size,
                                    vertices_loc));
                    }
                    break;
                }

                case core::ShapeKind::TRIANGLE: {
                    m_pass->enable_msaa = true;
                    auto mesh_size = layer_commons::get_mesh_size(
                        m_layer_ctx, {(long)shape_params.tri.side,
                                      (long)(shape_params.tri.side * 0.5 * sqrt(3))});

                    if (shape_params.fill || !(shape_params.border_size > 0)) {
                        m_pass->mesh = new TriangleMesh;
                        CHECK_AK_ERROR2(static_cast<TriangleMesh*>(m_pass->mesh)
                                            ->create(mesh_size, vertices_loc));
                    } else {
                        m_pass->mesh = new TriangleMesh;
                        CHECK_AK_ERROR2(
                            static_cast<TriangleMesh*>(m_pass->mesh)
                                ->create_border({mesh_size[0] - (float)shape_params.border_size,
                                                 mesh_size[1] - (float)shape_params.border_size},
                                                shape_params.border_size, vertices_loc));
                    }
                    break;
                }
                case core::ShapeKind::LINE: {
                    m_pass->mesh = new LineMesh;
                    CHECK_AK_ERROR2(static_cast<LineMesh*>(m_pass->mesh)
                                        ->create_default(vertices_loc, shape_params.line.size,
                                                         shape_params.line.begin,
                                                         shape_params.line.end));
                    break;
                }

                default: {
                    AKLOG_ERROR("Invalid or not implemented shape kind {} found", shape_kind);
                    break;
                }
            }

            return true;
        }

    }

}
