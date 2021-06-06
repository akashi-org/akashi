#pragma once

#include "../../gl.h"
#include "../../core/shader.h"

#include <libakcore/error.h>

namespace akashi {
    namespace core {
        enum class LayerType;
    }
    namespace graphics {

        struct GLRenderContext;

        struct LayerQuadPassProp {
            GLuint prog;

            GLuint vao;

            GLuint ibo;

            int ibo_length;

            GLuint tex_loc;

            GLuint mvp_loc;

            GLuint flipY_loc;

            GLuint time_loc;

            GLuint global_time_loc;

            GLuint local_duration_loc;

            GLuint resolution_loc;
        };

        class LayerQuadPass final {
          public:
            explicit LayerQuadPass(void) = default;
            virtual ~LayerQuadPass(void) = default;

            bool create(const GLRenderContext& ctx, const core::LayerContext& layer,
                        const core::LayerType& type);
            bool destroy(const GLRenderContext& ctx);
            const LayerQuadPassProp& get_prop() const;

            void shader_reload(const GLRenderContext& ctx, const core::LayerContext& layer,
                               const core::LayerType& type, const std::vector<const char*> paths);

          private:
            bool load_shader(const GLRenderContext& ctx, const GLuint prog,
                             const UserShaderSet& shader_set) const;
            bool load_vao(const GLRenderContext& ctx, const GLuint prog, GLuint& vao) const;
            bool load_ibo(const GLRenderContext& ctx, GLuint& ibo) const;

          private:
            LayerQuadPassProp m_prop;
            UserShaderSet m_shader_set;
        };

        struct LayerQuadMesh {
            GLTextureData tex;
            glm::mat4 mvp = glm::mat4(1.0f);
            int8_t flip_y = 1; // if -1, upside down
        };

        struct LayerQuadObjectProp {
            LayerQuadPass pass;
            LayerQuadMesh mesh;
        };

        class LayerQuadObject final {
          public:
            explicit LayerQuadObject(void) = default;
            virtual ~LayerQuadObject(void) = default;
            LayerQuadObject(LayerQuadObject&&) = default;

            void create(const GLRenderContext& ctx, const LayerQuadPass&& pass,
                        LayerQuadMesh&& mesh);
            void destroy(const GLRenderContext& ctx);

            const LayerQuadObjectProp& get_prop() const;

            LayerQuadObjectProp& get_prop_mut() { return m_prop; }

            void update_mesh(const GLRenderContext& ctx, LayerQuadMesh&& mesh);

          private:
            LayerQuadObjectProp m_prop;
        };

    }
}
