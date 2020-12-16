#pragma once

#include "../../gl.h"

#include <libakcore/error.h>

namespace akashi {
    namespace graphics {

        struct GLRenderContext;

        struct QuadPassProp {
            GLuint prog;

            GLuint vao;

            GLuint ibo;

            int ibo_length;

            GLuint tex_loc;

            GLuint mvp_loc;

            GLuint flipY_loc;
        };

        class QuadPass final {
          public:
            explicit QuadPass(void) = default;
            virtual ~QuadPass(void) = default;

            bool create(const GLRenderContext& ctx);
            bool destroy(const GLRenderContext& ctx);
            const QuadPassProp& get_prop() const;

          private:
            bool load_shader(const GLRenderContext& ctx, const GLuint prog) const;
            bool load_vao(const GLRenderContext& ctx, const GLuint prog, GLuint& vao) const;
            bool load_ibo(const GLRenderContext& ctx, GLuint& ibo) const;

          private:
            QuadPassProp m_prop;
        };

        struct QuadMesh {
            GLTextureData tex;
            glm::mat4 mvp = glm::mat4(1.0f);
            int8_t flip_y = 1; // if -1, upside down
        };

        struct QuadObjectProp {
            QuadPass pass;
            QuadMesh mesh;
        };

        class QuadObject final {
          public:
            explicit QuadObject(void) = default;
            virtual ~QuadObject(void) = default;
            QuadObject(QuadObject&&) = default;

            void create(const GLRenderContext& ctx, const QuadPass& pass, QuadMesh&& mesh);
            void destroy(const GLRenderContext& ctx);

            const QuadObjectProp& get_prop() const;
            void update_mesh(const GLRenderContext& ctx, QuadMesh&& mesh);

          private:
            QuadObjectProp m_prop;
        };

    }
}
