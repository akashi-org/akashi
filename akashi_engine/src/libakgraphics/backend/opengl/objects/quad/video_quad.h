#pragma once

#include "../../gl.h"

#include <libakcore/error.h>
#include <glm/glm.hpp>

namespace akashi {
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
        };

        class VideoQuadPass final {
          public:
            explicit VideoQuadPass(void) = default;
            virtual ~VideoQuadPass(void) = default;

            bool create(const GLRenderContext& ctx, const VTexSizeFormat& format);
            bool destroy(const GLRenderContext& ctx);
            bool need_update(const VTexSizeFormat& new_format) const;
            const VideoQuadPassProp& get_prop() const;

          private:
            bool load_shader(const GLRenderContext& ctx, const GLuint prog) const;
            bool load_vao(const GLRenderContext& ctx, const VTexSizeFormat& format,
                          const GLuint prog, GLuint& vao) const;
            bool load_ibo(const GLRenderContext& ctx, GLuint& ibo) const;

          private:
            VideoQuadPassProp m_prop;
            VTexSizeFormat m_size_format;
        };

        struct VideoQuadMesh {
            GLTextureData texY;
            GLTextureData texCb;
            GLTextureData texCr;
            glm::mat4 mvp = glm::mat4(1.0f);
            int8_t flip_y = 1; // if -1, upside down
        };

        struct VideoQuadObjectProp {
            VideoQuadPass pass;
            VideoQuadMesh mesh;
        };

        class VideoQuadObject final {
          public:
            explicit VideoQuadObject(void) = default;
            virtual ~VideoQuadObject(void) = default;
            VideoQuadObject(VideoQuadObject&&) = default;

            void create(const GLRenderContext& ctx, const VideoQuadPass&& pass,
                        VideoQuadMesh&& mesh);
            void destroy(const GLRenderContext& ctx);

            const VideoQuadObjectProp& get_prop() const;

            bool created(void) const;

            void update_pass(const GLRenderContext& ctx, VideoQuadPass&& pass);

            void update_mesh(const GLRenderContext& ctx, VideoQuadMesh&& mesh);

            bool need_update_pass(const VTexSizeFormat& format);

          private:
            VideoQuadObjectProp m_prop;
            bool m_created = false;
        };

    }
}
