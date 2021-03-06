#pragma once

#include "../../gl.h"
#include "../../core/shader.h"

#include <libakcore/error.h>
#include <libakcore/string.h>
#include <libakcore/hw_accel.h>
#include <libakcore/memory.h>
#include <libakbuffer/avbuffer.h>

#include <glm/glm.hpp>
#include <va/va.h>
#include <va/va_drmcommon.h>

namespace akashi {
    namespace core {
        struct LayerContext;
    }
    namespace graphics {

        class VTexSizeFormat {
          public:
            int video_width;
            int video_height;
            // [TODO] maybe luma_tex_width, chroma_tex_width are double type
            // but, it will get complex to do a comparison
            int luma_tex_width = 1;   // avoid 0 div
            int chroma_tex_width = 1; // avoid 0 div

            bool operator==(const VTexSizeFormat& other) const {
                // clang-format off
                return video_width == other.video_width 
                  && video_height == other.video_height 
                  && luma_tex_width == other.luma_tex_width 
                  && chroma_tex_width == other.chroma_tex_width;
                // clang-format on
            }

            bool operator!=(const VTexSizeFormat& other) const { return !(*this == other); }
        };

        struct VideoQuadPassProp {
            GLuint prog;

            GLuint vao;

            GLuint ibo;

            int ibo_length;

            GLuint texY_loc;

            GLuint texCb_loc;

            GLuint texCr_loc;

            GLuint mvp_loc;

            GLuint flipY_loc;

            GLuint time_loc;

            GLuint global_time_loc;

            GLuint local_duration_loc;

            GLuint fps_loc;

            GLuint resolution_loc;
        };

        class VideoQuadPass final {
          public:
            explicit VideoQuadPass(void) = default;
            virtual ~VideoQuadPass(void) = default;

            bool create(const GLRenderContext& ctx, const VTexSizeFormat& format,
                        const core::LayerContext& layer,
                        const core::VideoDecodeMethod& decode_method);
            bool destroy(const GLRenderContext& ctx);
            const VideoQuadPassProp& get_prop() const;

          private:
            bool load_shader(const GLRenderContext& ctx, const GLuint prog,
                             const core::LayerContext& layer,
                             const core::VideoDecodeMethod& decode_method) const;
            bool load_vao(const GLRenderContext& ctx, const VTexSizeFormat& format,
                          const GLuint prog, GLuint& vao) const;
            bool load_ibo(const GLRenderContext& ctx, GLuint& ibo) const;

          private:
            VideoQuadPassProp m_prop;
            VTexSizeFormat m_size_format;
            core::VideoDecodeMethod m_decode_method;
        };

        class VideoQuadMesh final {
          public:
            struct VAAPIContext {
                VAImage va_image;
                VADRMPRIMESurfaceDescriptor desc;
                EGLImage egl_images[3] = {nullptr};
                bool need_free = false;
            };

          public:
            explicit VideoQuadMesh(void) = default;
            virtual ~VideoQuadMesh(void) = default;

            bool create(const GLRenderContext& ctx, core::owned_ptr<buffer::AVBufferData> buf_data);

            bool destroy(const GLRenderContext& ctx);

            void update(const GLRenderContext& ctx, core::owned_ptr<buffer::AVBufferData> buf_data);

            const std::vector<GLTextureData>& textures(void) const { return m_textures; }
            const glm::mat4& mvp(void) const { return m_mvp; }
            int8_t flip_y(void) const { return m_flip_y; }

            core::VideoDecodeMethod decode_method(void) const { return m_decode_method; }

          private:
            template <enum core::VideoDecodeMethod>
            bool create_inner(const GLRenderContext& ctx, const buffer::AVBufferData& buf_data);

            void free_vaapi_context(const GLRenderContext& ctx);

          private:
            std::vector<GLTextureData> m_textures;
            glm::mat4 m_mvp = glm::mat4(1.0f);
            int8_t m_flip_y = 1; // if -1, upside down
            core::VideoDecodeMethod m_decode_method;
            core::owned_ptr<buffer::AVBufferData> m_buf_data = nullptr;
            VAAPIContext m_vaapi_ctx;
        };

        struct VideoQuadObjectProp {
            VideoQuadPass pass;
            VideoQuadMesh mesh;
        };

        class VideoQuadObject final {
          public:
            explicit VideoQuadObject(void) = default;
            virtual ~VideoQuadObject(void) = default;

            void create(const GLRenderContext& ctx, const VideoQuadPass&& pass,
                        core::owned_ptr<buffer::AVBufferData> buf_data);
            void destroy(const GLRenderContext& ctx);

            const VideoQuadObjectProp& get_prop() const;

            VideoQuadObjectProp& get_prop_mut() { return m_prop; }

            bool created(void) const;

            void update_pass(const GLRenderContext& ctx, VideoQuadPass&& pass);

            void update_mesh(const GLRenderContext& ctx,
                             core::owned_ptr<buffer::AVBufferData> buf_data);

          private:
            VideoQuadObjectProp m_prop;
            bool m_created = false;
        };

    }
}
