#pragma once

#include "./layer_commons.h"
#include "./layer_shaders.h"
#include "./layer_video_texture.h"
#include "../meshes/quad.h"
#include "../core/texture.h"

#include "../render_context.h"
#include "../fbo.h"

#include "../core/texture.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>

#include <SDL.h>
#include "../resource/image.h"
#include "../resource/font.h"

#include <libakvgfx/akvgfx.h>
#include <libakvgfx/item.h>

namespace akashi {
    namespace graphics::layer {

        namespace detail {

            inline std::array<float, 2> get_mesh_size(const std::array<long, 2>& layer_size,
                                                      const std::array<long, 2>& orig_size) {
                auto aspect_ratio =
                    core::Rational(orig_size[0], 1) / core::Rational(orig_size[1], 1);
                auto layer_width = layer_size[0];
                auto layer_height = layer_size[1];

                if (layer_width > 0 && layer_height > 0) {
                    return {(float)layer_width, (float)layer_height};
                } else if (layer_width > 0) {
                    return {(float)layer_width,
                            (float)(core::Rational(layer_width, 1) / aspect_ratio).to_decimal()};
                } else if (layer_height > 0) {
                    return {(float)(core::Rational(layer_height, 1) * aspect_ratio).to_decimal(),
                            (float)layer_height};
                }

                return {(float)orig_size[0], (float)orig_size[1]};
            }

        }

        inline std::array<float, 2> get_mesh_size(const core::LayerContext& layer_ctx,
                                                  const std::array<long, 2>& orig_size) {
            return detail::get_mesh_size(layer_ctx.t_transform->layer_size, orig_size);
        }

        inline std::array<float, 2> get_mesh_size(const layer::SafeLayerContext& safe_ctx,
                                                  const std::array<long, 2>& orig_size) {
            return detail::get_mesh_size(safe_ctx.layer_size, orig_size);
        }

        inline bool load_image_texture(layer::Texture* tex, OGLRenderContext& ctx,
                                       const core::LayerContext& layer_ctx) {
            const auto& srcs = layer_ctx.t_image->srcs;
            if (srcs.empty()) {
                AKLOG_ERRORN("Failed to getSurface. `srcs` is null");
                return false;
            }

            auto num_sprites = srcs.size();
            std::vector<SDL_Surface*> surfaces(num_sprites, nullptr);

            auto image_bits = 0;
            for (size_t i = 0; i < surfaces.size(); i++) {
                const auto& image_path = srcs[i];
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
                        const auto& first_path = srcs[0];
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

            tex->basic = new OGLTexture;
            tex->main_tex_loc = UniformLocation::image_textures;

            tex->basic->width = surfaces[0]->w;
            tex->basic->height = surfaces[0]->h;
            tex->basic->effective_width = surfaces[0]->w;
            tex->basic->effective_height = surfaces[0]->h;
            tex->basic->format = (surfaces[0]->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;
            tex->basic->internal_format =
                (surfaces[0]->format->BytesPerPixel == 3) ? GL_RGB8 : GL_RGBA8;
            tex->basic->target = GL_TEXTURE_2D_ARRAY;

            glGenTextures(1, &tex->basic->buffer);

            glBindTexture(GL_TEXTURE_2D_ARRAY, tex->basic->buffer);

            // 1. below GL4.2
            // GET_GLFUNC(ctx, glTexImage3D)
            // (GL_TEXTURE_2D_ARRAY, 0, tex.internal_format, tex.width, tex.height,
            // m_surfaces.size(),
            //  0, tex.format, GL_UNSIGNED_BYTE, nullptr);

            // 2. GL4.2+
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, tex->basic->internal_format, tex->basic->width,
                           tex->basic->height, surfaces.size());

            // GET_GLFUNC(ctx, glGenerateMipmap)(GL_TEXTURE_2D_ARRAY);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            for (size_t i = 0; i < surfaces.size(); i++) {
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, tex->basic->width,
                                tex->basic->height, 1, tex->basic->format, GL_UNSIGNED_BYTE,
                                surfaces[i]->pixels);
            }

            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

            for (auto&& surface : surfaces) {
                SDL_FreeSurface(surface);
            }

            return true;
        }

        inline bool load_text_texture(layer::Texture* tex, OGLRenderContext& ctx,
                                      const core::LayerContext& layer_ctx) {
            SDL_Surface* surface = nullptr;

            FontInfo info;
            info.text = layer_ctx.t_text->text;
            info.text_align = layer_ctx.t_text->text_align;
            info.pad = layer_ctx.t_text->pad;
            info.line_span = layer_ctx.t_text->line_span;

            core::TextStyleTField style;
            if (layer_ctx.t_text_style) {
                style = *layer_ctx.t_text_style;
            }
            info.color = hex_to_sdl(style.fg_color);
            auto font_path = style.font_path;
            info.font_path = font_path.empty() ? ctx.default_font_path() : font_path;
            info.size = style.fg_size <= 0 ? 0 : style.fg_size;

            FontOutline outline;
            outline.size = style.outline_size;
            outline.color = hex_to_sdl(style.outline_color);

            FontShadow shadow;
            shadow.color = hex_to_sdl(style.shadow_color);
            shadow.size = style.shadow_size;

            if (!FontLoader::GetInstance().get_surface(surface, info,
                                                       style.use_outline ? &outline : nullptr,
                                                       style.use_shadow ? &shadow : nullptr)) {
                AKLOG_ERRORN("Failed to getFontsSurface");
                return false;
            }

            tex->basic = new OGLTexture;
            tex->main_tex_loc = UniformLocation::text_texture0;

            tex->basic->image = surface->pixels;
            tex->basic->width = surface->w;
            tex->basic->height = surface->h;
            tex->basic->effective_width = tex->basic->width;
            tex->basic->effective_height = tex->basic->height;
            tex->basic->format = (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA;

            tex->basic->surface = surface;

            glGenTextures(1, &tex->basic->buffer);

            glBindTexture(GL_TEXTURE_2D, tex->basic->buffer);
            glTexImage2D(GL_TEXTURE_2D, 0, tex->basic->internal_format, tex->basic->width,
                         tex->basic->height, 0, tex->basic->format, GL_UNSIGNED_BYTE,
                         tex->basic->image);

            // glGenerateMipmap(GL_TEXTURE_2D);

            // [XXX] make sure to explicity setup when not using mimap
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);

            return true;
        }

        inline bool load_shape_texture(layer::Texture* tex, const vgfx::Surface& surface) {
            tex->basic = new OGLTexture;
            tex->main_tex_loc = UniformLocation::shape_texture0;

            tex->basic->image = surface.buffer();
            tex->basic->width = surface.info().width;
            tex->basic->height = surface.info().height;
            tex->basic->effective_width = tex->basic->width;
            tex->basic->effective_height = tex->basic->height;

            tex->basic->format = surface.info().format == vgfx::SurfaceFormat::RGB24
                                     ? (surface.info().format_swap ? GL_BGR : GL_RGB)
                                     : (surface.info().format_swap ? GL_BGRA : GL_RGBA);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            {
                glGenTextures(1, &tex->basic->buffer);

                glBindTexture(GL_TEXTURE_2D, tex->basic->buffer);

                int bytes_per_pixel = surface.info().format == vgfx::SurfaceFormat::RGB24 ? 3 : 4;
                glPixelStorei(GL_UNPACK_ROW_LENGTH, surface.info().stride / bytes_per_pixel);
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, tex->basic->internal_format, tex->basic->width,
                                 tex->basic->height, 0, tex->basic->format, GL_UNSIGNED_BYTE,
                                 tex->basic->image);
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

        inline bool load_shape_buffers(layer::Texture* tex, layer::Mesh* mesh,
                                       OGLRenderContext& ctx, const core::LayerContext& layer_ctx) {
            auto vertices_loc = InputLocation::vertices;
            auto uvs_loc = InputLocation::uvs;
            auto shape_kind = get_shape_kind(layer_ctx);
            switch (shape_kind) {
                case ShapeKind::RECT: {
                    const auto& shape_params = layer_ctx.t_rect;
                    std::array<float, 2> mesh_size = {(float)shape_params->width,
                                                      (float)shape_params->height};

                    const int border_padv = 4;
                    const float border_pad = shape_params->border_size * border_padv;

                    mesh->quad = new QuadMesh;

                    CHECK_AK_ERROR2(
                        mesh->quad->create({mesh_size[0] + border_pad, mesh_size[1] + border_pad},
                                           vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = border_padv},
                                                        layer_ctx);
                    CHECK_AK_ERROR2(load_shape_texture(tex, *surface));
                    break;
                }

                case ShapeKind::CIRCLE: {
                    auto layer_size = layer_ctx.t_transform->layer_size;
                    std::array<float, 2> mesh_size = {(float)layer_size[0], (float)layer_size[1]};

                    const int border_padv = 4;
                    const float border_pad = layer_ctx.t_circle->border_size * border_padv;

                    mesh->quad = new QuadMesh;

                    CHECK_AK_ERROR2(
                        mesh->quad->create({mesh_size[0] + border_pad, mesh_size[1] + border_pad},
                                           vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = border_padv},
                                                        layer_ctx);
                    CHECK_AK_ERROR2(load_shape_texture(tex, *surface));

                    break;
                }

                case ShapeKind::TRI: {
                    float mesh_width = layer_ctx.t_tri->width;
                    if (layer_ctx.t_tri->wr < 0 || layer_ctx.t_tri->wr > 1) {
                        mesh_width += (2 * std::abs(layer_ctx.t_tri->wr) * layer_ctx.t_tri->width);
                    }
                    float mesh_height = layer_ctx.t_tri->height;

                    const int border_padv = 4;
                    const float border_pad = layer_ctx.t_tri->border_size * border_padv;

                    std::array<float, 2> mesh_size = {mesh_width, mesh_height};

                    mesh->quad = new QuadMesh;

                    CHECK_AK_ERROR2(
                        mesh->quad->create({mesh_width + border_pad, mesh_height + border_pad},
                                           vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = border_padv},
                                                        layer_ctx);
                    CHECK_AK_ERROR2(load_shape_texture(tex, *surface));
                    break;
                }
                case ShapeKind::LINE: {
                    auto layer_size = layer_ctx.t_transform->layer_size;
                    std::array<float, 2> mesh_size = {(float)layer_size[0], (float)layer_size[1]};

                    mesh->quad = new QuadMesh;
                    CHECK_AK_ERROR2(mesh->quad->create(mesh_size, vertices_loc, uvs_loc));

                    auto surface = vgfx::create_surface({.width = (int)mesh_size[0],
                                                         .height = (int)mesh_size[1],
                                                         .format = vgfx::SurfaceFormat::ARGB32,
                                                         .border_padv = 0},
                                                        layer_ctx);
                    CHECK_AK_ERROR2(load_shape_texture(tex, *surface));

                    break;
                }

                default: {
                    AKLOG_ERROR("Invalid or not implemented shape kind {} found", shape_kind);
                    break;
                }
            }

            return true;
        }

        inline bool load_buffers(layer::Texture* tex, layer::Mesh* mesh, OGLRenderContext& ctx,
                                 const core::LayerContext& layer_ctx) {
            auto layer_type = layer::get_layer_type(layer_ctx);
            switch (layer_type) {
                case LayerType::IMAGE: {
                    CHECK_AK_ERROR2(load_image_texture(tex, ctx, layer_ctx));
                    auto mesh_size = get_mesh_size(
                        layer_ctx, {tex->basic->effective_width, tex->basic->effective_height});
                    mesh->quad = new QuadMesh;
                    CHECK_AK_ERROR2(
                        mesh->quad->create(mesh_size, InputLocation::vertices, InputLocation::uvs));
                    break;
                }
                case LayerType::TEXT: {
                    CHECK_AK_ERROR2(load_text_texture(tex, ctx, layer_ctx));
                    auto mesh_size = get_mesh_size(
                        layer_ctx, {tex->basic->effective_width, tex->basic->effective_height});
                    mesh->quad = new QuadMesh;
                    CHECK_AK_ERROR2(
                        mesh->quad->create(mesh_size, InputLocation::vertices, InputLocation::uvs));
                    break;
                }
                case LayerType::UNIT: {
                    auto layer_width = layer_ctx.t_transform->layer_size[0];
                    auto layer_height = layer_ctx.t_transform->layer_size[1];
                    auto mesh_size = get_mesh_size(layer_ctx, {layer_width, layer_height});
                    mesh->quad = new QuadMesh;
                    bool flip_uv = true;
                    CHECK_AK_ERROR2(mesh->quad->create(mesh_size, InputLocation::vertices,
                                                       InputLocation::uvs, flip_uv));
                    break;
                }
                case LayerType::SHAPE: {
                    return load_shape_buffers(tex, mesh, ctx, layer_ctx);
                }
                default: {
                    AKLOG_ERROR("Invalid layer type found: {}", layer_type);
                    return false;
                }
            }
            return true;
        }

        inline bool load_video_buffers(layer::Texture* tex, layer::Mesh* mesh,
                                       OGLRenderContext& ctx,
                                       const layer::SafeLayerContext& safe_ctx,
                                       core::owned_ptr<buffer::AVBufferData>&& buf_data) {
            tex->video = new VideoTexture;
            CHECK_AK_ERROR2(tex->video->create(ctx, std::move(buf_data)));

            const auto& vtex_info = tex->video->info();

            std::array<float, 2> mesh_size =
                get_mesh_size(safe_ctx, {vtex_info.video_width, vtex_info.video_height});

            mesh->quad = new QuadMesh;

            CHECK_AK_ERROR2(mesh->quad->create(mesh_size, vtex_info, InputLocation::vertices,
                                               InputLocation::luma_uvs, InputLocation::chroma_uvs));
            return true;
        }

    }

}
