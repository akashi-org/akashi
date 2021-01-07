#pragma once

#include <GL/gl.h>
#include <string>

namespace akashi {
    namespace core {
        struct LayerContext;
        enum class LayerType;
    }
    namespace graphics {

        struct GLRenderContext;

        bool compile_attach_shader(const GLRenderContext& ctx, const GLuint prog, const GLenum type,
                                   const char* source);

        bool link_shader(const GLRenderContext& ctx, const GLuint prog);

        struct UserShader {
            std::string path;
            std::string body;
        };

        class UserShaderSet final {
          public:
            explicit UserShaderSet(void) = default;
            virtual ~UserShaderSet(void);

            void load(const core::LayerContext& layer, const core::LayerType& type);

            bool contains(const std::string& path);

            const UserShader& frag(void) const { return m_frag; }

            const UserShader& geom(void) const { return m_geom; }

          private:
            template <GLenum shader_type>
            void read_shader(UserShader& user_shader, const core::LayerContext& layer,
                             const core::LayerType& layer_type);

          private:
            UserShader m_frag;
            UserShader m_geom;
        };

    }
}
