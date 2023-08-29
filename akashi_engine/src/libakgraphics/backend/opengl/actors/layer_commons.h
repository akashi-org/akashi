#pragma once

#include "../core/glc.h"
#include "../core/texture.h"
#include "../core/shader.h"
#include "../meshes/quad.h"

#include <libakcore/element.h>
#include <libakcore/error.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <stdexcept>

#define GET_SHADER(shader_type, layer, layer_type)                                                 \
    [](const akashi::core::LayerContext& layer_, const akashi::core::LayerType& type_) {           \
        switch (type_) {                                                                           \
            case core::LayerType::VIDEO: {                                                         \
                return layer_.video_layer_ctx.shader_type;                                         \
            }                                                                                      \
            case core::LayerType::TEXT: {                                                          \
                return layer_.text_layer_ctx.shader_type;                                          \
            }                                                                                      \
            case core::LayerType::IMAGE: {                                                         \
                return layer_.image_layer_ctx.shader_type;                                         \
            }                                                                                      \
            case core::LayerType::EFFECT: {                                                        \
                return layer_.effect_layer_ctx.shader_type;                                        \
            }                                                                                      \
            default: {                                                                             \
                AKLOG_ERROR("Not implemented Error for the type: {}", type_);                      \
                throw std::runtime_error("Not implemented Error");                                 \
            }                                                                                      \
        }                                                                                          \
    }((layer), (layer_type))

namespace akashi {
    namespace graphics {

        namespace layer_commons {

            enum UniformLocation {
                // public
                time = 100,
                global_time,
                location_duration,
                fps,
                resolution,
                mesh_size,
                unit_texture0,
                text_texture0,
                shape_texture0,
                image_textures,
                video_textureY,
                video_textureCb,
                video_textureCr,

                // private
                mvpMatrix = 200,
                uv_flip_hv,
                main_tex_kind
            };

            enum InputLocation { vertices = 0, uvs };

            static const std::string shader_header_ = u8R"(
    #version 420 core
    #extension GL_ARB_explicit_uniform_location : require
            )";

            static const std::string uniform_decls_ = u8R"(
    layout (location = 100) uniform float time;
    layout (location = 101) uniform float global_time;
    layout (location = 102) uniform float local_duration;
    layout (location = 103) uniform float fps;
    layout (location = 104) uniform vec2 resolution;
    layout (location = 105) uniform vec2 mesh_size;
    layout (location = 106) uniform sampler2D unit_texture0;
    layout (location = 107) uniform sampler2D text_texture0;
    layout (location = 108) uniform sampler2D shape_texture0;
    layout (location = 109) uniform sampler2DArray image_textures;
    layout (location = 110) uniform sampler2D video_textureY;
    layout (location = 111) uniform sampler2D video_textureCb;
    layout (location = 112) uniform sampler2D video_textureCr;

    layout (location = 200) uniform mat4 mvpMatrix;
    layout (location = 201) uniform ivec2 uv_flip_hv; // [uv_flip_h, uv_flip_v]
    layout (location = 202) uniform float main_tex_kind;
    )";

            static const std::string vshader_src = shader_header_ + uniform_decls_ + u8R"(
    layout (location = 0) in vec3 vertices;
    layout (location = 1) in vec2 uvs;


    out VS_OUT {
        vec2 video_luma_uv;
        vec2 video_chroma_uv;
        vec2 uv;
        float sprite_idx;
    } vs_out;

    void poly_main(inout vec4 pos);

    vec2 get_uvs(){
        return vec2(
            uv_flip_hv[0] == 1 ? 1.0 - uvs.x : uvs.x,
            uv_flip_hv[1] == 1 ? 1.0 - uvs.y : uvs.y
        );
    }
    
    void main(void){
        vs_out.uv = get_uvs();
        vs_out.sprite_idx = 0;
        vec4 t_vertices = vec4(vertices, 1.0);
        poly_main(t_vertices);
        gl_Position = mvpMatrix * t_vertices;
    }
)";

            static const std::string fshader_src = shader_header_ + uniform_decls_ + u8R"(

    in GS_OUT {
        vec2 video_luma_uv;
        vec2 video_chroma_uv;
        vec2 uv;
        float sprite_idx;
    } fs_in;

    out vec4 fragColor;

    void frag_main(inout vec4 rv);

    vec4 get_color() {
        switch(int(main_tex_kind)){
            // // video
            // case 0: {
            //     return texture(text_texture0, fs_in.uv);
            // }
            case 2: {
                return texture(text_texture0, fs_in.uv);
            }
            case 3: {
                return texture(image_textures, vec3(fs_in.uv, fs_in.sprite_idx));
            }
            case 4: {
                return texture(unit_texture0, fs_in.uv);
            }
            case 5: {
                return texture(shape_texture0, fs_in.uv);
            }
            default:
                return vec4(0.0);
        }
    }

    void main(void){
        vec4 smpColor = get_color();
        fragColor = smpColor;
        frag_main(fragColor);
    }
)";

            static const std::string default_user_pshader_src =
                shader_header_ + uniform_decls_ + u8R"(
    void poly_main(inout vec4 position){
    }
)";

            static const std::string default_user_fshader_src =
                shader_header_ + uniform_decls_ + u8R"(
    void frag_main(inout vec4 _fragColor){
    }
)";

            static const std::string default_user_gshader_src = u8R"(
    #version 420 core
    layout (triangles) in;
    layout (triangle_strip, max_vertices = 3) out;

    in VS_OUT {
        vec2 video_luma_uv;
        vec2 video_chroma_uv;
        vec2 uv;
        float sprite_idx;
    } gs_in[];

    out GS_OUT {
        vec2 video_luma_uv;
        vec2 video_chroma_uv;
        vec2 uv;
        float sprite_idx;
    } gs_out;

    void main() {
        for(int i = 0; i < 3; i++){
            gs_out.uv = gs_in[i].uv;
            gs_out.sprite_idx = gs_in[i].sprite_idx;
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
)";

            struct CommonProgramLocation {
                GLuint mvp_loc;

                GLuint time_loc;

                GLuint global_time_loc;

                GLuint local_duration_loc;

                GLuint fps_loc;

                GLuint resolution_loc;

                GLuint mesh_size_loc;
            };

            struct Transform {
                glm::vec3 trans_vec = glm::vec3(1.0f);

                glm::vec3 scale_vec = glm::vec3(1.0f);

                core::Rational rotation = core::Rational(0, 1);

                glm::mat4 model_mat = glm::mat4(1.0f);
            };

            inline bool load_shaders(const GLuint prog, const core::LayerType& /*type*/,
                                     const std::string& u_poly_shader,
                                     const std::string& u_frag_shader) {
                CHECK_AK_ERROR2(compile_attach_shader(prog, GL_VERTEX_SHADER, vshader_src.c_str()));
                CHECK_AK_ERROR2(
                    compile_attach_shader(prog, GL_FRAGMENT_SHADER, fshader_src.c_str()));

                std::string frag_shader =
                    u_frag_shader.empty() ? default_user_fshader_src : u_frag_shader;
                CHECK_AK_ERROR2(
                    compile_attach_shader(prog, GL_FRAGMENT_SHADER, frag_shader.c_str()));

                std::string poly_shader =
                    u_poly_shader.empty() ? default_user_pshader_src : u_poly_shader;
                CHECK_AK_ERROR2(compile_attach_shader(prog, GL_VERTEX_SHADER, poly_shader.c_str()));

                CHECK_AK_ERROR2(compile_attach_shader(prog, GL_GEOMETRY_SHADER,
                                                      default_user_gshader_src.c_str()));

                CHECK_AK_ERROR2(link_shader(prog));
                return true;
            }

            inline glm::vec3 get_trans_vec(const std::array<double, 3>& layer_pos) {
                GLint viewport[4];
                glGetIntegerv(GL_VIEWPORT, viewport);
                int screen_width = viewport[2];
                int screen_height = viewport[3];

                auto c_x = core::Rational(screen_width, 2);
                auto c_y = core::Rational(screen_height, 2);

                auto a_x = core::Rational(layer_pos[0]); // mouse coord
                auto a_y = core::Rational(layer_pos[1]); // mouse coord

                return glm::vec3((a_x - c_x).to_decimal(), -(a_y - c_y).to_decimal(), layer_pos[2]);
            }

            inline std::array<float, 2> get_mesh_size(const core::LayerContext& layer_ctx,
                                                      const std::array<long, 2>& orig_size) {
                auto aspect_ratio =
                    core::Rational(orig_size[0], 1) / core::Rational(orig_size[1], 1);

                if (layer_ctx.layer_size[0] > 0 && layer_ctx.layer_size[1] > 0) {
                    return {(float)layer_ctx.layer_size[0], (float)layer_ctx.layer_size[1]};
                } else if (layer_ctx.layer_size[0] > 0) {
                    return {(float)layer_ctx.layer_size[0],
                            (float)(core::Rational(layer_ctx.layer_size[0], 1) / aspect_ratio)
                                .to_decimal()};
                } else if (layer_ctx.layer_size[1] > 0) {
                    return {(float)(core::Rational(layer_ctx.layer_size[1], 1) * aspect_ratio)
                                .to_decimal(),
                            (float)layer_ctx.layer_size[1]};
                }

                return {(float)orig_size[0], (float)orig_size[1]};
            }

            inline void update_model_mat(Transform* pass, const core::LayerContext& layer_ctx) {
                pass->model_mat = glm::translate(pass->model_mat, pass->trans_vec);
                pass->model_mat = glm::rotate(pass->model_mat,
                                              glm::radians((float)layer_ctx.rotation.to_decimal()),
                                              glm::vec3(0.0f, 0.0f, 1.0f));
                pass->model_mat = glm::scale(pass->model_mat, pass->scale_vec);
            }

        }

    }
}
