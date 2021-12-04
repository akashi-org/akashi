#pragma once

#include "../core/eglc.h"

#include <libakcore/hw_accel.h>
#include <libakcore/memory.h>

#include <va/va.h>
#include <va/va_drmcommon.h>

#include <vector>

namespace akashi {

    namespace buffer {
        class AVBufferData;
    }
    namespace core {
        struct VideoLayerContext;
    }
    namespace graphics {

        class OGLRenderContext;
        class OGLTexture;

        struct VideoTextureInfo {
            int video_width;
            int video_height;
            // [TODO] maybe luma_tex_width, chroma_tex_width are double type
            // but, it will get complex to do a comparison
            int luma_tex_width = 1;   // avoid 0 div
            int chroma_tex_width = 1; // avoid 0 div

            // bool operator==(const VideoTextureInfo& other) const {
            //     // clang-format off
            //     return video_width == other.video_width
            //       && video_height == other.video_height
            //       && luma_tex_width == other.luma_tex_width
            //       && chroma_tex_width == other.chroma_tex_width;
            //     // clang-format on
            // }

            // bool operator!=(const VideoTextureInfo& other) const { return !(*this == other); }
        };

        class VideoTexture final {
            struct HWContext;

          public:
            explicit VideoTexture(void);
            virtual ~VideoTexture(void);

            bool create(const OGLRenderContext& ctx, const core::VideoLayerContext& vlayer_ctx,
                        core::owned_ptr<buffer::AVBufferData>&& buf_data);

            bool destroy();

            bool update(const OGLRenderContext& ctx, const core::VideoLayerContext& vlayer_ctx,
                        core::owned_ptr<buffer::AVBufferData>&& buf_data);

            void use_textures(const std::array<GLuint, 3>& tex_locs);

            const std::vector<OGLTexture>& textures(void) const { return m_textures; }

            const VideoTextureInfo& info(void) const { return m_info; }

            core::VideoDecodeMethod decode_method(void) const { return m_decode_method; }

          private:
            bool create_inner(const core::VideoDecodeMethod& decode_method, const OGLRenderContext&,
                              const core::VideoLayerContext&, const buffer::AVBufferData&);

            bool create_inner_sw(const OGLRenderContext&, const core::VideoLayerContext&,
                                 const buffer::AVBufferData&);

            bool create_inner_vaapi(const OGLRenderContext&, const core::VideoLayerContext&,
                                    const buffer::AVBufferData&);

            bool create_inner_vaapi_copy(const OGLRenderContext&, const core::VideoLayerContext&,
                                         const buffer::AVBufferData&);

            void free_vaapi_context();

            void update_texture_info(const buffer::AVBufferData& buf_data);

          private:
            std::vector<OGLTexture> m_textures;
            core::VideoDecodeMethod m_decode_method;
            core::owned_ptr<buffer::AVBufferData> m_buf_data;
            VideoTextureInfo m_info;
            HWContext* m_hwctx = nullptr;
        };

    }

}
