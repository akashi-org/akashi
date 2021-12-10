#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        struct VideoTextureInfo;

        class QuadMesh final : public BaseMesh {
          public:
            explicit QuadMesh() : BaseMesh(){};
            virtual ~QuadMesh() = default;

            bool create(const std::array<float, 2>& size, const GLuint vertices_loc,
                        const GLuint uvs_loc, const bool flip_uv = false);

            bool create(const std::array<float, 2>& size, const VideoTextureInfo& info,
                        const GLuint vertices_loc, const GLuint luma_uvs_loc,
                        const GLuint chroma_uvs_loc);

            void destroy() override;
        };

    }
}
