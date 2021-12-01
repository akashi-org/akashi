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

            bool create(int fbo_width, int fbo_height);

            bool render(const glm::mat4& pv) const;

            void destroy();

            bool initilized() const { return m_initialized; }

            const FBInfo& info() const;

            bool texture(OGLTexture& in_tex) const;

          private:
            bool load_pass();
            bool load_texture();
            bool load_fbo();

          private:
            Pass* m_pass = nullptr;
            FBInfo m_info;
            bool m_initialized = false;
        };
    }

}
