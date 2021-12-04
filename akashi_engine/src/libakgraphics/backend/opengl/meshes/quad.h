#pragma once

#include "../core/glc.h"

#include <cstddef>
#include <array>

namespace akashi {
    namespace graphics {

        struct VideoTextureInfo;

        class QuadMesh final {
          public:
            explicit QuadMesh() = default;
            virtual ~QuadMesh() = default;

            bool create(const std::array<float, 2>& size, const GLuint vertices_loc,
                        const GLuint uvs_loc, const bool flip_uv = false);

            bool create(const std::array<float, 2>& size, const VideoTextureInfo& info,
                        const GLuint vertices_loc, const GLuint luma_uvs_loc,
                        const GLuint chroma_uvs_loc);

            void destroy();

            GLuint vao() const { return m_vao; }

            GLuint ibo() const { return m_ibo; }

            size_t ibo_length() const { return m_ibo_length; }

          private:
            GLuint m_vao;
            GLuint m_ibo;
            size_t m_ibo_length = 0;
        };

    }
}
