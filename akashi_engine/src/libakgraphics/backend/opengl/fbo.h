#pragma once

#include "./core/glc.h"
#include <glm/glm.hpp>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace graphics {

        class OGLRenderContext;
        struct OGLTexture;

        struct FBInfo {
            GLuint fbo;
            int width;
            int height;
        };

        class FBO final {
            struct Pass;

          public:
            const static GLuint FBO_TEX_UNIT = 0;

          public:
            explicit FBO() = default;
            virtual ~FBO() = default;

            bool create(int fbo_width, int fbo_height, int msaa = 0, bool enable_alpha = true);

            bool render(OGLRenderContext& ctx);

            void resolve() const;

            void destroy();

            bool initilized() const { return m_initialized; }

            const FBInfo& info() const;

            bool dst_fbo(GLuint* fbo) const;

            bool texture(OGLTexture& in_tex) const;

            bool msaa_texture(OGLTexture& in_tex) const;

          private:
            bool load_pass();

            bool load_texture();

            bool load_msaa_texture(int msaa);

            bool load_fbo(int msaa);

            glm::mat4 get_model_mat() const;

            glm::vec3 get_sar_scale_vec(const OGLTexture& tex) const;

          private:
            Pass* m_pass = nullptr;
            FBInfo m_info;
            bool m_initialized = false;
            bool m_enable_alpha = true;
        };
    }

}
