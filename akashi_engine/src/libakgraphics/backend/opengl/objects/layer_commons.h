#pragma once

#include "../core/glc.h"

#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

namespace akashi {
    namespace graphics {

        class QuadMesh;
        class VideoTexture;
        struct OGLTexture;

        namespace layer {

            struct Mesh {
                QuadMesh* quad = nullptr;
            };

            struct Texture {
                VideoTexture* video = nullptr;
                OGLTexture* basic = nullptr;
                GLuint main_tex_loc = 100;
            };

            struct Transform {
                glm::vec3 trans_vec = glm::vec3(1.0f);
                glm::vec3 scale_vec = glm::vec3(1.0f);
                float rotation_rad = 0.0f;
                glm::mat4 model_mat = glm::mat4(1.0f);
            };

            struct SafeLayerContext {
                std::string uuid;
                std::string atom_uuid;
                std::string key;
                bool display = false;
                core::Rational from = core::Rational(0, 1);
                core::Rational layer_local_offset = core::Rational(0, 1);
                core::Rational to = core::Rational(0, 1);

                // [XXX] For load_video_buffers()
                std::array<long, 2> layer_size = {-1, -1};
            };

            enum class LayerType { VIDEO = 0, AUDIO, TEXT, IMAGE, UNIT, SHAPE, LENGTH };

            inline LayerType get_layer_type(const core::LayerContext& layer_type) {
                if (layer_type.t_video) {
                    return LayerType::VIDEO;
                } else if (layer_type.t_text) {
                    return LayerType::TEXT;
                } else if (layer_type.t_image) {
                    return LayerType::IMAGE;
                } else if (layer_type.t_unit) {
                    return LayerType::UNIT;
                } else if (layer_type.t_rect || layer_type.t_circle || layer_type.t_tri ||
                           layer_type.t_line) {
                    return LayerType::SHAPE;
                } else {
                    return LayerType::LENGTH;
                }
            }

            enum class ShapeKind { RECT = 0, CIRCLE, TRI, LINE, LENGTH };

            inline ShapeKind get_shape_kind(const core::LayerContext& layer_type) {
                if (layer_type.t_rect) {
                    return ShapeKind::RECT;
                } else if (layer_type.t_circle) {
                    return ShapeKind::CIRCLE;
                } else if (layer_type.t_tri) {
                    return ShapeKind::TRI;
                } else if (layer_type.t_line) {
                    return ShapeKind::LINE;
                } else {
                    return ShapeKind::LENGTH;
                }
            }

        }

    }
}
