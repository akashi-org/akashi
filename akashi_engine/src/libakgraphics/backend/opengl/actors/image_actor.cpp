#include "./image_actor.h"

#include "./layer_commons.h"
#include "../render_context.h"
#include "../camera.h"
#include "../resource/image.h"
#include "../fbo.h"
#include "../core/texture.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>

#include <SDL.h>

namespace akashi {
    namespace graphics {

        struct ImageActor::Pass : public layer_commons::CommonProgramLocation,
                                  public layer_commons::Transform {
            GLuint prog;
            QuadMesh mesh;
            GLuint tex_loc;
            OGLTexture tex;
        };

        bool ImageActor::create(OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            if (m_pass) {
                AKLOG_ERRORN("Pass already loaded");
                return false;
            }
            m_pass = new ImageActor::Pass;
            CHECK_AK_ERROR2(this->load_pass(ctx));
            return true;
        }

        bool ImageActor::render(OGLRenderContext& ctx, const core::Rational& pts,
                                const Camera& camera) {
            glUseProgram(m_pass->prog);

            use_ogl_texture(m_pass->tex, m_pass->tex_loc);

            glm::mat4 new_mvp = camera.vp_mat() * m_pass->model_mat;

            glUniformMatrix4fv(m_pass->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            auto local_pts = (pts - m_layer_ctx.from) + m_layer_ctx.layer_local_offset;
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

        bool ImageActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                m_pass->mesh.destroy();
                // free_ogl_texture(m_pass->tex);
                glDeleteTextures(1, &m_pass->tex.buffer);
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;

            return true;
        }

        bool ImageActor::load_pass(const OGLRenderContext& ctx) {
            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(layer_commons::load_shaders(m_pass->prog, m_layer_type,
                                                        m_layer_ctx.image_layer_ctx.poly,
                                                        m_layer_ctx.image_layer_ctx.frag));
            glUseProgram(m_pass->prog);

            m_pass->mvp_loc = layer_commons::UniformLocation::mvpMatrix;
            m_pass->tex_loc = layer_commons::UniformLocation::image_textures;
            m_pass->time_loc = layer_commons::UniformLocation::time;
            m_pass->global_time_loc = layer_commons::UniformLocation::global_time;
            m_pass->local_duration_loc = layer_commons::UniformLocation::location_duration;
            m_pass->fps_loc = layer_commons::UniformLocation::fps;
            m_pass->resolution_loc = layer_commons::UniformLocation::resolution;
            m_pass->mesh_size_loc = layer_commons::UniformLocation::mesh_size;

            auto vertices_loc = layer_commons::InputLocation::vertices;
            auto uvs_loc = layer_commons::InputLocation::uvs;

            CHECK_AK_ERROR2(this->load_texture(ctx));

            std::array<long, 2> mesh_orig_size = {m_pass->tex.effective_width,
                                                  m_pass->tex.effective_height};

            QuadMeshCrop mesh_crop;
            auto crop_end = m_layer_ctx.image_layer_ctx.crop.end;
            bool do_crop = crop_end[0] != 0 || crop_end[1] != 0;
            if (do_crop) {
                auto crop_begin = m_layer_ctx.image_layer_ctx.crop.begin;
                auto crop_width = crop_end[0] - crop_begin[0];
                auto crop_height = crop_end[1] - crop_begin[1];
                if (crop_width > 0 and crop_height > 0) {
                    mesh_orig_size[0] = crop_width;
                    mesh_orig_size[1] = crop_height;
                }

                mesh_crop.begin = crop_begin;
                mesh_crop.end = crop_end;
                mesh_crop.orig_width = m_pass->tex.effective_width;
                mesh_crop.orig_height = m_pass->tex.effective_height;
            }

            auto mesh_size = layer_commons::get_mesh_size(m_layer_ctx, mesh_orig_size);

            CHECK_AK_ERROR2(m_pass->mesh.create(mesh_size, vertices_loc, uvs_loc, false,
                                                do_crop ? &mesh_crop : nullptr));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            m_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.image_layer_ctx.scale;

            layer_commons::update_model_mat(m_pass, m_layer_ctx);

            {
                glUseProgram(m_pass->prog);

                // [XXX]
                // These are unused textures in this layer.
                // But if we don't set the arbitrary value for those,
                // GL_INVALID_OPERATION will occur in glDrawElements.
                glUniform1i(layer_commons::UniformLocation::text_texture0, 2);
                glUniform1i(layer_commons::UniformLocation::unit_texture0, 3);
                glUniform1i(layer_commons::UniformLocation::shape_texture0, 4);

                glUniform2i(layer_commons::UniformLocation::uv_flip_hv, m_layer_ctx.uv_flip_h,
                            m_layer_ctx.uv_flip_v);

                glUniform1f(layer_commons::UniformLocation::main_tex_kind,
                            static_cast<float>(m_layer_type));

                glUniform2fv(m_pass->mesh_size_loc, 1, m_pass->mesh.mesh_size().data());
                glUseProgram(0);
            }

            return true;
        }

        bool ImageActor::load_texture(const OGLRenderContext& /* ctx */) {
            if (m_layer_ctx.image_layer_ctx.srcs.empty()) {
                AKLOG_ERRORN("Failed to getSurface. `image_layer_ctx.srcs` is null");
                return false;
            }

            auto num_sprites = m_layer_ctx.image_layer_ctx.srcs.size();
            std::vector<SDL_Surface*> surfaces(num_sprites, nullptr);

            auto image_bits = 0;
            for (size_t i = 0; i < surfaces.size(); i++) {
                const auto& image_path = m_layer_ctx.image_layer_ctx.srcs[i];
                if (ImageLoader::GetInstance().getSurface(surfaces[i], image_path.c_str()) !=
                    core::ErrorType::OK) {
                    AKLOG_ERROR("Failed to getSurface: {}", image_path.c_str());
                    return false;
                }

                auto cur_image_bits = surfaces[i]->format->BytesPerPixel;
                if (i == 0) {
                    image_bits = cur_image_bits;
                } else {
                    if (image_bits != cur_image_bits) {
                        const auto& first_path = m_layer_ctx.image_layer_ctx.srcs[0];
                        AKLOG_ERROR("Image channel mismatch found: `{}`(bits={}) != `{}`(bits={})",
                                    first_path.c_str(), image_bits, image_path.c_str(),
                                    cur_image_bits);
                        for (auto&& surface : surfaces) {
                            SDL_FreeSurface(surface);
                        }
                        return false;
                    }
                }
            }

            m_pass->tex.width = surfaces[0]->w;
            m_pass->tex.height = surfaces[0]->h;
            m_pass->tex.effective_width = surfaces[0]->w;
            m_pass->tex.effective_height = surfaces[0]->h;
            m_pass->tex.format = (surfaces[0]->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            m_pass->tex.internal_format =
                (surfaces[0]->format->BytesPerPixel == 3) ? GL_RGB8 : GL_RGBA8;
            m_pass->tex.target = GL_TEXTURE_2D_ARRAY;

            glGenTextures(1, &m_pass->tex.buffer);

            glBindTexture(GL_TEXTURE_2D_ARRAY, m_pass->tex.buffer);

            // 1. below GL4.2
            // GET_GLFUNC(ctx, glTexImage3D)
            // (GL_TEXTURE_2D_ARRAY, 0, tex.internal_format, tex.width, tex.height,
            // m_surfaces.size(),
            //  0, tex.format, GL_UNSIGNED_BYTE, nullptr);

            // 2. GL4.2+
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, m_pass->tex.internal_format, m_pass->tex.width,
                           m_pass->tex.height, surfaces.size());

            // GET_GLFUNC(ctx, glGenerateMipmap)(GL_TEXTURE_2D_ARRAY);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            for (size_t i = 0; i < surfaces.size(); i++) {
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, m_pass->tex.width,
                                m_pass->tex.height, 1, m_pass->tex.format, GL_UNSIGNED_BYTE,
                                surfaces[i]->pixels);
            }

            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

            for (auto&& surface : surfaces) {
                SDL_FreeSurface(surface);
            }

            return true;
        }

    }

}
