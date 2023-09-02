#pragma once

#include <string>

namespace akashi {
    namespace graphics::layer {

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
            main_tex_kind,
            video_decode_method,
        };

        enum InputLocation { vertices = 0, uvs, luma_uvs, chroma_uvs };

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
    layout (location = 203) uniform float video_decode_method;
    )";

        static const std::string vshader_src = shader_header_ + uniform_decls_ + u8R"(
    layout (location = 0) in vec3 vertices;
    layout (location = 1) in vec2 uvs;
    layout (location = 2) in vec2 luma_uvs;
    layout (location = 3) in vec2 chroma_uvs;

    out VS_OUT {
        vec2 video_luma_uv;
        vec2 video_chroma_uv;
        vec2 uv;
        float sprite_idx;
    } vs_out;

    void poly_main(inout vec4 pos);

    vec2 get_uvs(vec2 raw_uvs){
        return vec2(
            uv_flip_hv[0] == 1 ? 1.0 - raw_uvs.x : raw_uvs.x,
            uv_flip_hv[1] == 1 ? 1.0 - raw_uvs.y : raw_uvs.y
        );
    }
    
    void main(void){
        if(int(main_tex_kind) == 0){
            vs_out.video_luma_uv = get_uvs(luma_uvs);
            vs_out.video_chroma_uv = get_uvs(chroma_uvs);
        }else {
            vs_out.uv = get_uvs(uvs);
        }
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

    vec3 get_yuv() {
        switch(int(video_decode_method)){
            // sw
            case 0: {
                return vec3(
                    texture(video_textureY, fs_in.video_luma_uv).r,
                    texture(video_textureCb, fs_in.video_chroma_uv).r,
                    texture(video_textureCr, fs_in.video_chroma_uv).r
                );
            }
            // vaapi
            case 1: {
                return vec3(
                    texture(video_textureY, fs_in.video_luma_uv).r,
                    texture(video_textureCb, fs_in.video_luma_uv).r,
                    texture(video_textureCb, fs_in.video_luma_uv).g
                );
            }
            default: {
                return vec3(0.0);
            }
        }
    }

    vec4 to_rgb(){
        
        // MPEG, YUV(Y: 16~235, UV: 16~240) => RGB(RGB: 0~255)
        vec3 yuv = get_yuv();
        yuv.y -= 0.5;
        yuv.z -= 0.5;

        // BT.601
        // vec3 rc = vec3(1, 0, 1.402);
        // vec3 gc = vec3(1,-0.344136, -0.714136);
        // vec3 bc = vec3(1, 1.772, 0);
        
        // BT.709
        vec3 rc = vec3(1, 0, 1.5748);
        vec3 gc = vec3(1,-0.187324, -0.468124);
        vec3 bc = vec3(1, 1.8556, 0);

        return vec4(
            (dot(yuv, rc)-0.0625) * 1.164, 
            (dot(yuv, gc)-0.0625) * 1.164, 
            (dot(yuv, bc)-0.0625) * 1.164, 
            1
        );
    }

    void frag_main(inout vec4 rv);

    vec4 get_color() {
        switch(int(main_tex_kind)){
            // video
            case 0: {
                return to_rgb();
            }
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

        static const std::string default_user_pshader_src = shader_header_ + uniform_decls_ + u8R"(
    void poly_main(inout vec4 position){
    }
)";

        static const std::string default_user_fshader_src = shader_header_ + uniform_decls_ + u8R"(
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
            gs_out.video_luma_uv = gs_in[i].video_luma_uv; 
            gs_out.video_chroma_uv = gs_in[i].video_chroma_uv;
            gs_out.uv = gs_in[i].uv;
            gs_out.sprite_idx = gs_in[i].sprite_idx;
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
)";

    }
}
