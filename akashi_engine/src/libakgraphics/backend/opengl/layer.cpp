#include "./layer.h"

#include "./context.h"
#include "./gl.h"
#include "./core/texture.h"
#include "./core/mvp.h"
#include "./objects/quad/quad.h"
#include "./resource/font.h"
#include "./resource/image.h"

#include <libakbuffer/avbuffer.h>
#include <libakcore/logger.h>
#include <libakcore/error.h>
#include <libakcore/memory.h>
#include <libakcore/element.h>

#include <SDL.h>
#include <string>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        static bool layer_render(const GLRenderContext& ctx, const QuadPassProp& pass_prop,
                                 const QuadMesh& mesh_prop, const LayerContext& layer_ctx) {
            GET_GLFUNC(ctx, glUseProgram)(pass_prop.prog);

            use_texture(ctx, mesh_prop.tex, pass_prop.tex_loc);

            glm::mat4 new_mvp = mesh_prop.mvp;
            if (static_cast<LayerType>(layer_ctx.type) == LayerType::TEXT) {
                update_translate(ctx, layer_ctx, new_mvp);
            }
            update_scale(ctx, mesh_prop.tex, new_mvp);

            GET_GLFUNC(ctx, glUniformMatrix4fv)
            (pass_prop.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.flipY_loc, mesh_prop.flip_y);

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
                                       const LayerContext& layer_ctx) {
            GET_GLFUNC(ctx, glUseProgram)(pass_prop.prog);

            use_texture(ctx, mesh_prop.texY, pass_prop.texY_loc);
            use_texture(ctx, mesh_prop.texCb, pass_prop.texCb_loc);
            use_texture(ctx, mesh_prop.texCr, pass_prop.texCr_loc);

            glm::mat4 new_mvp = mesh_prop.mvp;
            update_translate(ctx, layer_ctx, new_mvp);
            // [XXX] looks confusing, but we only need to process the texY,
            // because update_scale gets the magnification rate from the size of the tex, and
            // reflect it to mvp
            update_scale(ctx, mesh_prop.texY, new_mvp);

            GET_GLFUNC(ctx, glUniformMatrix4fv)
            (pass_prop.mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            GET_GLFUNC(ctx, glUniform1f)(pass_prop.flipY_loc, mesh_prop.flip_y);

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
                    CHECK_AK_ERROR2(video_layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx));

                    AKLOG_INFON("VideoLayerTarget::render(): rendered by using the last frame");
                }
                return true;
            }

            auto loop_cnt = glx_ctx->loop_cnt();
            auto buf_data = glx_ctx->dequeue(m_layer_ctx.uuid + std::to_string(loop_cnt),
                                             glx_ctx->current_time());
            // when dequeue failed, render the last frame
            // and, in that case, when the last frame does not exist, do nothing
            if (!buf_data) {
                AKLOG_INFON("VideoLayerTarget::render(): dequeue failed");

                if (m_quad_obj.created()) {
                    const auto& obj_prop = m_quad_obj.get_prop();
                    const auto& pass_prop = obj_prop.pass.get_prop();
                    const auto& mesh_prop = obj_prop.mesh;
                    CHECK_AK_ERROR2(video_layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx));
                    AKLOG_INFON("VideoLayerTarget::render(): rendered by using the last frame");
                }
                return true;
            }

            // [TODO] it stinks around here
            VTexSizeFormat size_format;
            size_format.video_width = buf_data->prop().width;
            size_format.video_height = buf_data->prop().height;
            size_format.luma_tex_width = buf_data->prop().video_data[0].stride;
            // [TODO] not sure why this calculation is valid
            size_format.chroma_tex_width = buf_data->prop().video_data[1].stride *
                                           buf_data->prop().width / buf_data->prop().chroma_width;

            if (!m_quad_obj.created()) {
                VideoQuadPass pass;
                pass.create(ctx, size_format);

                VideoQuadMesh mesh;
                CHECK_AK_ERROR2(this->load_mesh(ctx, mesh, m_layer_ctx, *buf_data));
                m_quad_obj.create(ctx, std::move(pass), std::move(mesh));

            } else {
                if (m_quad_obj.need_update_pass(size_format)) {
                    VideoQuadPass pass;
                    pass.create(ctx, size_format);
                    m_quad_obj.update_pass(ctx, std::move(pass));
                }
                VideoQuadMesh mesh;
                CHECK_AK_ERROR2(this->load_mesh(ctx, mesh, m_layer_ctx, *buf_data));
                m_quad_obj.update_mesh(ctx, std::move(mesh));
            }

            const auto& obj_prop = m_quad_obj.get_prop();
            const auto& pass_prop = obj_prop.pass.get_prop();
            const auto& mesh_prop = obj_prop.mesh;

            CHECK_AK_ERROR2(video_layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx));

            m_current_pts = pts;
            return true;
        }

        bool VideoLayerTarget::destroy(const GLRenderContext& ctx) {
            m_quad_obj.destroy(ctx);
            return true;
        }

        bool VideoLayerTarget::load_mesh(const GLRenderContext& ctx, VideoQuadMesh& mesh,
                                         const core::LayerContext& layer_ctx,
                                         const buffer::AVBufferData& buf_data) const {
            CHECK_AK_ERROR2(this->load_texture(ctx, mesh.texY, layer_ctx, buf_data, 0));
            CHECK_AK_ERROR2(this->load_texture(ctx, mesh.texCb, layer_ctx, buf_data, 1));
            CHECK_AK_ERROR2(this->load_texture(ctx, mesh.texCr, layer_ctx, buf_data, 2));
            return true;
        }

        bool VideoLayerTarget::load_texture(const GLRenderContext& ctx, GLTextureData& tex,
                                            const core::LayerContext&,
                                            const buffer::AVBufferData& buf_data,
                                            const int vdata_index) const {
            tex.image = buf_data.prop().video_data[vdata_index].buf;
            tex.width = buf_data.prop().video_data[vdata_index].stride - 1;
            tex.height = vdata_index == 0 ? buf_data.prop().height : buf_data.prop().chroma_height;
            tex.effective_width =
                vdata_index == 0 ? buf_data.prop().width : buf_data.prop().chroma_width;
            tex.effective_height = tex.height;
            tex.format = GL_RED;
            tex.internal_format = GL_R8;

            // [TODO] could this be duplicate with the other indices
            tex.index = vdata_index;

            // [TODO] error-check?
            create_texture(ctx, tex);

            return true;
        }

        /* --- TextLayerTarget --- */

        bool TextLayerTarget::create(const GLRenderContext& ctx, core::LayerContext layer_ctx) {
            m_layer_ctx = layer_ctx;

            QuadMesh mesh;
            CHECK_AK_ERROR2(this->load_mesh(ctx, mesh, m_layer_ctx));

            // [TODO] nullptr check
            m_quad_obj.create(ctx, *ctx.pass, std::move(mesh));

            return true;
        }

        bool TextLayerTarget::render(core::borrowed_ptr<GLGraphicsContext>,
                                     const GLRenderContext& ctx, const core::Rational&) {
            const auto& obj_prop = m_quad_obj.get_prop();
            const auto& pass_prop = obj_prop.pass.get_prop();
            const auto& mesh_prop = obj_prop.mesh;
            CHECK_AK_ERROR2(layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx));
            return true;
        }

        bool TextLayerTarget::destroy(const GLRenderContext& ctx) {
            m_quad_obj.destroy(ctx);
            return true;
        }

        bool TextLayerTarget::load_mesh(const GLRenderContext& ctx, QuadMesh& mesh,
                                        core::LayerContext& layer_ctx) const {
            CHECK_AK_ERROR2(this->load_texture(ctx, mesh.tex, layer_ctx));
            return true;
        }

        bool TextLayerTarget::load_texture(const GLRenderContext& ctx, GLTextureData& tex,
                                           core::LayerContext& layer_ctx) const {
            // [XXX] json::optional::unwrap_or internally calls std::map[], which is not defined for
            // the const value, so we must use non-const value when using unwrap_or

            SDL_Surface* surface = nullptr;

            FontInfo info;
            info.text = layer_ctx.text_layer_ctx.text.c_str();

            // [TODO] not sure what happens when style is blank. check is needed.
            auto style = json::optional::unwrap_or(layer_ctx.text_layer_ctx.style);

            auto fill = json::optional::unwrap_or(style.fill);
            auto fg_color = hex_to_sdl(fill);
            info.fg = fg_color;

            info.font_path = ctx.default_font_path.c_str();

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

            QuadMesh mesh;
            CHECK_AK_ERROR2(this->load_mesh(ctx, mesh, m_layer_ctx));

            // [TODO] nullptr check
            m_quad_obj.create(ctx, *ctx.pass, std::move(mesh));

            return true;
        }

        bool ImageLayerTarget::render(core::borrowed_ptr<GLGraphicsContext>,
                                      const GLRenderContext& ctx, const core::Rational&) {
            const auto& obj_prop = m_quad_obj.get_prop();
            const auto& pass_prop = obj_prop.pass.get_prop();
            const auto& mesh_prop = obj_prop.mesh;
            CHECK_AK_ERROR2(layer_render(ctx, pass_prop, mesh_prop, m_layer_ctx));
            return true;
        }

        bool ImageLayerTarget::destroy(const GLRenderContext& ctx) {
            m_quad_obj.destroy(ctx);
            return true;
        }

        bool ImageLayerTarget::load_mesh(const GLRenderContext& ctx, QuadMesh& mesh,
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
            tex.effective_height = surface->w;
            tex.format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            tex.surface = surface;

            // [TODO] error-check?
            create_texture(ctx, tex);

            return true;
        }
    }
}
