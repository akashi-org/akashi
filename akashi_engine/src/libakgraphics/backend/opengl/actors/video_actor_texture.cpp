#include "./video_actor_texture.h"

#include "../render_context.h"
#include "../fbo.h"
#include "../core/texture.h"
#include "../core/eglc.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/hw_accel.h>
#include <libakbuffer/avbuffer.h>

#include <libdrm/drm_fourcc.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <unistd.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        struct VideoTexture::HWContext {
            VAImage va_image;
            VADRMPRIMESurfaceDescriptor desc;
            EGLImage egl_images[3] = {nullptr};
            bool needs_free = false;
        };

        VideoTexture::VideoTexture() {
            m_hwctx = new VideoTexture::HWContext;
            m_buf_data = nullptr;
        }

        VideoTexture::~VideoTexture() {
            if (m_hwctx) {
                delete m_hwctx;
            }
        }

        bool VideoTexture::create(const OGLRenderContext& ctx,
                                  const core::VideoLayerContext& vlayer_ctx,
                                  core::owned_ptr<buffer::AVBufferData>&& buf_data) {
            m_buf_data = std::move(buf_data);
            this->update_texture_info(*m_buf_data);
            m_textures.reserve(3);
            m_textures.resize(3);
            for (auto&& tex : m_textures) {
                glGenTextures(1, &tex.buffer);
            }

            m_decode_method = m_buf_data->prop().decode_method;
            return this->create_inner(m_decode_method, ctx, vlayer_ctx, *m_buf_data);
        }

        bool VideoTexture::destroy() {
            for (auto&& tex : m_textures) {
                free_ogl_texture(tex);
            }
            if (m_hwctx->needs_free) {
                this->free_vaapi_context();
            }
            if (m_buf_data) {
                m_buf_data.reset(nullptr);
            }
            return true;
        }

        bool VideoTexture::update(const OGLRenderContext& ctx,
                                  const core::VideoLayerContext& vlayer_ctx,
                                  core::owned_ptr<buffer::AVBufferData>&& buf_data) {
            if (m_decode_method == VideoDecodeMethod::VAAPI) {
                // [XXX] must be called before updating m_buf_data
                this->free_vaapi_context();
            }
            m_buf_data = std::move(buf_data);
            this->update_texture_info(*m_buf_data);

            return this->create_inner(m_decode_method, ctx, vlayer_ctx, *m_buf_data);
        }

        void VideoTexture::use_textures(const std::array<GLuint, 3>& tex_locs) {
            use_ogl_texture(m_textures[0], tex_locs[0]);
            use_ogl_texture(m_textures[1], tex_locs[1]);
            if (m_decode_method == VideoDecodeMethod::SW) {
                use_ogl_texture(m_textures[2], tex_locs[2]);
            }
        }

        bool VideoTexture::create_inner(const core::VideoDecodeMethod& decode_method,
                                        const OGLRenderContext& ctx,
                                        const core::VideoLayerContext& vlayer_ctx,
                                        const buffer::AVBufferData& buf_data) {
            switch (decode_method) {
                case VideoDecodeMethod::SW: {
                    return this->create_inner_sw(ctx, vlayer_ctx, buf_data);
                }
                case VideoDecodeMethod::VAAPI: {
                    return this->create_inner_vaapi(ctx, vlayer_ctx, buf_data);
                }
                case VideoDecodeMethod::VAAPI_COPY: {
                    return this->create_inner_vaapi_copy(ctx, vlayer_ctx, buf_data);
                }
                default: {
                    return false;
                }
            }
        }

        bool VideoTexture::create_inner_sw(const OGLRenderContext& ctx,
                                           const core::VideoLayerContext& vlayer_ctx,
                                           const buffer::AVBufferData& buf_data) {
            for (size_t i = 0; i < m_textures.size(); i++) {
                auto& tex = m_textures[i];
                tex.image = buf_data.prop().video_data[i].buf;
                tex.width = buf_data.prop().video_data[i].stride - 1;
                tex.height = i == 0 ? buf_data.prop().height : buf_data.prop().chroma_height;
                tex.effective_width = i == 0 ? buf_data.prop().width : buf_data.prop().chroma_width;
                tex.effective_height = tex.height;

                if (vlayer_ctx.stretch) {
                    tex.effective_width = ctx.fbo().info().width;
                    tex.effective_height = ctx.fbo().info().height;
                }

                tex.format = GL_RED;
                tex.internal_format = GL_R8;

                // [TODO] could this be duplicate with the other indices
                tex.index = i;

                glBindTexture(GL_TEXTURE_2D, tex.buffer);
                glTexImage2D(GL_TEXTURE_2D, 0, tex.internal_format, tex.width, tex.height, 0,
                             tex.format, GL_UNSIGNED_BYTE, tex.image);

                // [XXX] make sure to explicity setup when not using mimap
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glBindTexture(GL_TEXTURE_2D, 0);
            }
            return true;
        }

        bool VideoTexture::create_inner_vaapi(const OGLRenderContext& ctx,
                                              const core::VideoLayerContext& vlayer_ctx,
                                              const buffer::AVBufferData& buf_data) {
            if (auto status = vaExportSurfaceHandle(
                    buf_data.prop().va_display, buf_data.prop().va_surface_id,
                    VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                    VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                    &m_hwctx->desc);
                status != 0) {
                AKLOG_ERROR("vaExportSurfaceHandle() failed: {}", vaErrorStr(status));
                return false;
            }

            if (auto status =
                    vaSyncSurface(buf_data.prop().va_display, buf_data.prop().va_surface_id);
                status != 0) {
                AKLOG_ERROR("vaSyncSurface() failed: {}", vaErrorStr(status));
                return false;
            }
            m_hwctx->needs_free = true;

            for (uint32_t i = 0; i < m_hwctx->desc.num_layers; i++) {
                if (m_hwctx->desc.layers[i].num_planes > 1) {
                    AKLOG_ERRORN("multiplane layer is not supported");
                    return false;
                }

                auto& tex = m_textures[i];
                tex.index = i;

                switch (m_hwctx->desc.layers[i].drm_format) {
                    case DRM_FORMAT_R8: {
                        tex.format = GL_RED;
                        tex.internal_format = GL_R8;
                        tex.width = m_hwctx->desc.width;
                        tex.height = m_hwctx->desc.height;
                        tex.effective_width = buf_data.prop().width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    case DRM_FORMAT_RG88: {
                        tex.format = GL_RG;
                        tex.internal_format = GL_RG8;
                        tex.width = buf_data.prop().chroma_width;
                        tex.height = buf_data.prop().chroma_height;
                        tex.effective_width = tex.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    case DRM_FORMAT_GR88: {
                        tex.format = GL_RG;
                        tex.internal_format = GL_RG8;
                        tex.reversed = 1;
                        tex.width = buf_data.prop().chroma_width;
                        tex.height = buf_data.prop().chroma_height;
                        tex.effective_width = tex.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    default: {
                        AKLOG_ERROR("not supported drm format found: {}",
                                    m_hwctx->desc.layers[i].drm_format);
                        return false;
                    }
                }

                if (vlayer_ctx.stretch) {
                    tex.effective_width = ctx.fbo().info().width;
                    tex.effective_height = ctx.fbo().info().height;
                }

                glBindTexture(GL_TEXTURE_2D, tex.buffer);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glBindTexture(GL_TEXTURE_2D, 0);

                auto obj_idx = m_hwctx->desc.layers[i].object_index[0]; // singleplane layer
                EGLuint64KHR modifier = m_hwctx->desc.objects[obj_idx].drm_format_modifier;

                // clang-format off
                    EGLint attribs[] = {
                        EGL_WIDTH, tex.width,
                        EGL_HEIGHT, tex.height,
                        EGL_LINUX_DRM_FOURCC_EXT, (EGLint)m_hwctx->desc.layers[i].drm_format,
                        EGL_DMA_BUF_PLANE0_FD_EXT, m_hwctx->desc.objects[obj_idx].fd,
                        EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)m_hwctx->desc.layers[i].offset[0], // singleplane layer
                        EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)m_hwctx->desc.layers[i].pitch[0], // singleplane layer
                        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(modifier & 0xffffffff),
                        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(modifier >> 32),
                        EGL_NONE
                    };
                // clang-format on

                // clang-format off
                    m_hwctx->egl_images[i] =  eglCreateImageKHR(
                      eglGetCurrentDisplay(),
                        EGL_NO_CONTEXT,
                        EGL_LINUX_DMA_BUF_EXT,
                        nullptr,
                        attribs
                    );
                // clang-format on

                if (!m_hwctx->egl_images[i]) {
                    AKLOG_ERROR("eglCreateImageKHR() failed: {}", eglGetError());
                    return false;
                }

                glBindTexture(GL_TEXTURE_2D, tex.buffer);
                glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_hwctx->egl_images[i]);
            }

            glBindTexture(GL_TEXTURE_2D, 0);
            return true;
        }

        bool VideoTexture::create_inner_vaapi_copy(const OGLRenderContext& ctx,
                                                   const core::VideoLayerContext& vlayer_ctx,
                                                   const buffer::AVBufferData& buf_data) {
            for (size_t i = 0; i < 2; i++) {
                auto& tex = m_textures[i];
                tex.image = buf_data.prop().video_data[i].buf;
                tex.width = i == 0 ? buf_data.prop().video_data[i].stride - 1
                                   : buf_data.prop().chroma_width;
                tex.height = i == 0 ? buf_data.prop().height : buf_data.prop().chroma_height;
                tex.effective_width = i == 0 ? buf_data.prop().width : buf_data.prop().chroma_width;
                tex.effective_height = tex.height;

                if (vlayer_ctx.stretch) {
                    tex.effective_width = ctx.fbo().info().width;
                    tex.effective_height = ctx.fbo().info().height;
                }

                tex.format = i == 0 ? GL_RED : GL_RG;
                tex.internal_format = i == 0 ? GL_R8 : GL_RG8;

                // [TODO] could this be duplicate with the other indices
                tex.index = i;

                glBindTexture(GL_TEXTURE_2D, tex.buffer);
                glTexImage2D(GL_TEXTURE_2D, 0, tex.internal_format, tex.width, tex.height, 0,
                             tex.format, GL_UNSIGNED_BYTE, tex.image);

                // [XXX] make sure to explicity setup when not using mimap
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glBindTexture(GL_TEXTURE_2D, 0);
            }
            return true;
        }

        void VideoTexture::free_vaapi_context() {
            if (m_hwctx->needs_free) {
                for (uint32_t i = 0; i < m_hwctx->desc.num_layers; i++) {
                    auto obj_idx = m_hwctx->desc.layers[i].object_index[0]; // singleplane layer
                    close(m_hwctx->desc.objects[obj_idx].fd);
                }
                for (int i = 0; i < 3; i++) {
                    if (m_hwctx->egl_images[i]) {
                        eglDestroyImageKHR(eglGetCurrentDisplay(), m_hwctx->egl_images[i]);
                    }
                    m_hwctx->egl_images[i] = nullptr;
                }
                m_hwctx->needs_free = false;
            }
        }

        void VideoTexture::update_texture_info(const buffer::AVBufferData& buf_data) {
            m_info.video_width = buf_data.prop().width;
            m_info.video_height = buf_data.prop().height;
            m_info.luma_tex_width = buf_data.prop().width;
            // m_info.luma_tex_width = buf_data.prop().video_data[0].stride;
            // [TODO] not sure why this calculation is valid
            m_info.chroma_tex_width = buf_data.prop().video_data[1].stride * buf_data.prop().width /
                                      buf_data.prop().chroma_width;
        }

    }

}
