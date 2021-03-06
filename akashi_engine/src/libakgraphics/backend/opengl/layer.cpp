#include "./layer.h"

#include "./context.h"
#include "./gl.h"
#include "./core/texture.h"
#include "./core/mvp.h"
#include "./objects/quad/layer_quad.h"
#include "./resource/font.h"
#include "./resource/image.h"

#include <libakbuffer/avbuffer.h>
#include <libakcore/logger.h>
#include <libakcore/error.h>
#include <libakcore/memory.h>
#include <libakcore/element.h>

#include <SDL.h>
#include <string>
#include <array>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        static bool layer_render(const GLRenderContext& ctx, const LayerQuadPassProp& pass_prop,
                                 const LayerQuadMesh& mesh_prop, const LayerContext& layer_ctx,
                                 const core::Rational& pts, const std::array<int, 2>& resolution,
                                 const core::Rational& fps) {
            GET_GLFUNC(ctx, glUseProgram)(pass_prop.prog);

            use_texture(ctx, mesh_prop.tex, pass_prop.tex_loc);

            glm::mat4 new_mvp = mesh_prop.mvp;
            switch (static_cast<LayerType>(layer_ctx.type)) {
                case LayerType::TEXT: {
                    update_translate(ctx, layer_ctx, new_mvp);
                    update_scale(ctx, mesh_prop.tex, new_mvp, layer_ctx.text_layer_ctx.scale);
                    break;
                }
                case LayerType::IMAGE: {
                    update_translate(ctx, layer_ctx, new_mvp);
                    update_scale(ctx, mesh_prop.tex, new_mvp, layer_ctx.image_layer_ctx.scale);
                    break;
                }
                default: {
                    break;
                }
            }

            GET_GLFUNC(ctx, glUniformMatrix4fv)
            (pass_prop.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.flipY_loc, mesh_prop.flip_y);

            auto local_pts = pts - to_rational(layer_ctx.from);
            GET_GLFUNC(ctx, glUniform1f)(pass_prop.time_loc, local_pts.to_decimal());
            GET_GLFUNC(ctx, glUniform1f)(pass_prop.global_time_loc, pts.to_decimal());

            auto local_duration = to_rational(layer_ctx.to) - to_rational(layer_ctx.from);
            GET_GLFUNC(ctx, glUniform1f)(pass_prop.local_duration_loc, local_duration.to_decimal());

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.fps_loc, fps.to_decimal());

            GET_GLFUNC(ctx, glUniform2f)(pass_prop.resolution_loc, resolution[0], resolution[1]);

            GET_GLFUNC(ctx, glBindVertexArray)(pass_prop.vao);
            // [XXX] required when using glDrawElements
            GET_GLFUNC(ctx, glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, pass_prop.ibo);

            GET_GLFUNC(ctx, glDrawElements)
            (GL_TRIANGLES, pass_prop.ibo_length, GL_UNSIGNED_SHORT, 0);

            GET_GLFUNC(ctx, glBindVertexArray)(0);

            return true;
        }

        static bool video_layer_render(const GLRenderContext& ctx,
                                       const VideoQuadPassProp& pass_prop,
                                       const VideoQuadMesh& mesh_prop,
                                       const LayerContext& layer_ctx, const core::Rational& pts,
                                       const std::array<int, 2>& resolution,
                                       const core::Rational& fps) {
            GET_GLFUNC(ctx, glUseProgram)(pass_prop.prog);

            use_texture(ctx, mesh_prop.textures()[0], pass_prop.texY_loc);
            use_texture(ctx, mesh_prop.textures()[1], pass_prop.texCb_loc);
            if (mesh_prop.decode_method() == VideoDecodeMethod::SW) {
                use_texture(ctx, mesh_prop.textures()[2], pass_prop.texCr_loc);
            }

            glm::mat4 new_mvp = mesh_prop.mvp();
            update_translate(ctx, layer_ctx, new_mvp);
            // [XXX] looks confusing, but we only need to process the texY,
            // because update_scale gets the magnification rate from the size of the tex, and
            // reflect it to mvp
            update_scale(ctx, mesh_prop.textures()[0], new_mvp, layer_ctx.video_layer_ctx.scale);

            GET_GLFUNC(ctx, glUniformMatrix4fv)
            (pass_prop.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.flipY_loc, mesh_prop.flip_y());

            auto local_pts = pts - to_rational(layer_ctx.from);
            GET_GLFUNC(ctx, glUniform1f)(pass_prop.time_loc, local_pts.to_decimal());
            GET_GLFUNC(ctx, glUniform1f)(pass_prop.global_time_loc, pts.to_decimal());

            auto local_duration = to_rational(layer_ctx.to) - to_rational(layer_ctx.from);
            GET_GLFUNC(ctx, glUniform1f)(pass_prop.local_duration_loc, local_duration.to_decimal());

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.fps_loc, fps.to_decimal());

            GET_GLFUNC(ctx, glUniform2f)(pass_prop.resolution_loc, resolution[0], resolution[1]);

            GET_GLFUNC(ctx, glBindVertexArray)(pass_prop.vao);
            // [XXX] required when using glDrawElements
            GET_GLFUNC(ctx, glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, pass_prop.ibo);

            GET_GLFUNC(ctx, glDrawElements)
            (GL_TRIANGLES, pass_prop.ibo_length, GL_UNSIGNED_SHORT, 0);

            GET_GLFUNC(ctx, glBindVertexArray)(0);

            return true;
        }

        /* --- VideoLayerTarget --- */

        bool VideoLayerTarget::create(const GLRenderContext&, core::LayerContext layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = core::LayerType::VIDEO;
            return true;
        }

        bool VideoLayerTarget::render(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                                      const GLRenderContext& ctx, const core::Rational& pts) {
            if (m_current_pts == pts) {
                AKLOG_INFON("VideoLayerTarget::render(): pts unchanged");

                if (m_quad_obj.created()) {
                    const auto& obj_prop = m_quad_obj.get_prop();
                    const auto& pass_prop = obj_prop.pass.get_prop();
                    const auto& mesh_prop = obj_prop.mesh;
                    CHECK_AK_ERROR2(video_layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx, pts,
                                                       glx_ctx->resolution(), glx_ctx->fps()));

                    AKLOG_INFON("VideoLayerTarget::render(): rendered by using the last frame");
                }
                return true;
            }

            auto loop_cnt = glx_ctx->loop_cnt();
            auto buf_data = glx_ctx->dequeue(m_layer_ctx.uuid + std::to_string(loop_cnt), pts);

            // when dequeue failed, render the last frame
            // and, in that case, when the last frame does not exist, do nothing
            if (!buf_data) {
                AKLOG_INFON("VideoLayerTarget::render(): dequeue failed");

                if (m_quad_obj.created()) {
                    const auto& obj_prop = m_quad_obj.get_prop();
                    const auto& pass_prop = obj_prop.pass.get_prop();
                    const auto& mesh_prop = obj_prop.mesh;
                    CHECK_AK_ERROR2(video_layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx, pts,
                                                       glx_ctx->resolution(), glx_ctx->fps()));
                    AKLOG_INFON("VideoLayerTarget::render(): rendered by using the last frame");
                }
                return true;
            }

            // [TODO] it stinks around here
            VTexSizeFormat size_format;
            size_format.video_width = buf_data->prop().width;
            size_format.video_height = buf_data->prop().height;
            size_format.luma_tex_width = buf_data->prop().width;
            // size_format.luma_tex_width = buf_data->prop().video_data[0].stride;
            // [TODO] not sure why this calculation is valid
            size_format.chroma_tex_width = buf_data->prop().video_data[1].stride *
                                           buf_data->prop().width / buf_data->prop().chroma_width;

            if (!m_quad_obj.created()) {
                VideoQuadPass pass;
                pass.create(ctx, size_format, m_layer_ctx, buf_data->prop().decode_method);
                m_quad_obj.create(ctx, std::move(pass), std::move(buf_data));
            } else {
                m_quad_obj.update_mesh(ctx, std::move(buf_data));
            }

            const auto& obj_prop = m_quad_obj.get_prop();
            const auto& pass_prop = obj_prop.pass.get_prop();
            const auto& mesh_prop = obj_prop.mesh;

            CHECK_AK_ERROR2(video_layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx, pts,
                                               glx_ctx->resolution(), glx_ctx->fps()));

            m_current_pts = pts;
            return true;
        }

        bool VideoLayerTarget::destroy(const GLRenderContext& ctx) {
            m_quad_obj.destroy(ctx);
            return true;
        }

        /* --- TextLayerTarget --- */

        bool TextLayerTarget::create(const GLRenderContext& ctx, core::LayerContext layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = core::LayerType::TEXT;

            LayerQuadMesh mesh;
            mesh.flip_y = ctx.layer_flip_y;
            CHECK_AK_ERROR2(this->load_mesh(ctx, mesh, m_layer_ctx));

            LayerQuadPass pass;
            pass.create(ctx, m_layer_ctx, m_layer_type);

            // [TODO] nullptr check
            m_quad_obj.create(ctx, std::move(pass), std::move(mesh));

            return true;
        }

        bool TextLayerTarget::render(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                                     const GLRenderContext& ctx, const core::Rational& pts) {
            const auto& obj_prop = m_quad_obj.get_prop();
            const auto& pass_prop = obj_prop.pass.get_prop();
            const auto& mesh_prop = obj_prop.mesh;
            CHECK_AK_ERROR2(layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx, pts,
                                         glx_ctx->resolution(), glx_ctx->fps()));
            return true;
        }

        bool TextLayerTarget::destroy(const GLRenderContext& ctx) {
            m_quad_obj.destroy(ctx);
            return true;
        }

        bool TextLayerTarget::load_mesh(const GLRenderContext& ctx, LayerQuadMesh& mesh,
                                        core::LayerContext& layer_ctx) const {
            CHECK_AK_ERROR2(this->load_texture(ctx, mesh.tex, layer_ctx));
            return true;
        }

        bool TextLayerTarget::load_texture(const GLRenderContext& ctx, GLTextureData& tex,
                                           core::LayerContext& layer_ctx) const {
            // [XXX] json::optional::unwrap_or internally calls std::map[], which is not defined
            // for the const value, so we must use non-const value when using unwrap_or

            SDL_Surface* surface = nullptr;

            FontInfo info;
            info.text = layer_ctx.text_layer_ctx.text.c_str();

            // [TODO] not sure what happens when style is blank. check is needed.
            auto style = json::optional::unwrap_or(layer_ctx.text_layer_ctx.style);

            auto fill = json::optional::unwrap_or(style.fill);
            auto fg_color = hex_to_sdl(fill);
            info.fg = fg_color;

            auto font_path = json::optional::unwrap_or(style.font_path);
            info.font_path = font_path.empty() ? ctx.default_font_path.c_str() : font_path.c_str();

            auto font_size = json::optional::unwrap_or(style.font_size);
            int default_font_size = 12;
            info.font_size = font_size <= 0 ? default_font_size : font_size;

            if (FontLoader::GetInstance().getSurface(surface, info) != ErrorType::OK) {
                AKLOG_ERRORN("TextLayerTarget::load_texture() failed: Failed to getFontsSurface");
                return false;
            }

            tex.image = surface->pixels;
            tex.width = surface->w;
            tex.height = surface->h;
            tex.effective_width = tex.width;
            tex.effective_height = tex.height;
            tex.format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            tex.surface = surface;

            // [TODO] error-check?
            create_texture(ctx, tex);

            return true;
        }

        /* --- ImageLayerTarget --- */

        bool ImageLayerTarget::create(const GLRenderContext& ctx, core::LayerContext layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = core::LayerType::IMAGE;

            LayerQuadMesh mesh;
            mesh.flip_y = ctx.layer_flip_y;
            CHECK_AK_ERROR2(this->load_mesh(ctx, mesh, m_layer_ctx));

            LayerQuadPass pass;
            pass.create(ctx, m_layer_ctx, m_layer_type);

            // [TODO] nullptr check
            m_quad_obj.create(ctx, std::move(pass), std::move(mesh));

            return true;
        }

        bool ImageLayerTarget::render(core::borrowed_ptr<GLGraphicsContext> glx_ctx,
                                      const GLRenderContext& ctx, const core::Rational& pts) {
            const auto& obj_prop = m_quad_obj.get_prop();
            const auto& pass_prop = obj_prop.pass.get_prop();
            const auto& mesh_prop = obj_prop.mesh;
            CHECK_AK_ERROR2(layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx, pts,
                                         glx_ctx->resolution(), glx_ctx->fps()));
            return true;
        }

        bool ImageLayerTarget::destroy(const GLRenderContext& ctx) {
            m_quad_obj.destroy(ctx);
            return true;
        }

        bool ImageLayerTarget::load_mesh(const GLRenderContext& ctx, LayerQuadMesh& mesh,
                                         core::LayerContext& layer_ctx) const {
            CHECK_AK_ERROR2(this->load_texture(ctx, mesh.tex, layer_ctx));
            return true;
        }

        bool ImageLayerTarget::load_texture(const GLRenderContext& ctx, GLTextureData& tex,
                                            core::LayerContext& layer_ctx) const {
            SDL_Surface* surface = nullptr;

            const char* image_path = layer_ctx.image_layer_ctx.src.c_str();
            if (ImageLoader::GetInstance().getSurface(surface, image_path) != ErrorType::OK) {
                AKLOG_ERRORN("load_texture failed: Failed to getSurface");
                return false;
            }

            tex.image = surface->pixels;
            tex.width = surface->w;
            tex.height = surface->h;
            tex.effective_width = surface->w;
            tex.effective_height = surface->h;
            tex.format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            tex.surface = surface;

            // [TODO] error-check?
            create_texture(ctx, tex);

            return true;
        }
    }
}
