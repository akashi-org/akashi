#pragma once

#include "./mesh.h"

#include <array>

namespace akashi {
    namespace graphics {

        struct VideoTextureInfo;

        struct QuadMeshCrop {
            std::array<long, 2> begin;
            std::array<long, 2> end;
            long orig_width;
            long orig_height;
        };

        class QuadMesh final : public BaseMesh {
          public:
            explicit QuadMesh() : BaseMesh(){};
            virtual ~QuadMesh() = default;

            bool create(const std::array<float, 2>& size, const GLuint vertices_loc,
                        const GLuint uvs_loc, const bool flip_uv = false,
                        const QuadMeshCrop* crop = nullptr);

            bool create(const std::array<float, 2>& size, const VideoTextureInfo& info,
                        const GLuint vertices_loc, const GLuint luma_uvs_loc,
                        const GLuint chroma_uvs_loc);

          protected:
            void destroy_inner() override;
        };

    }
}
