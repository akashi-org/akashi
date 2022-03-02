#include "./video_actor.h"

#include "./video_actor_texture.h"

#include "./layer_commons.h"
#include "../render_context.h"
#include "../camera.h"
#include "../fbo.h"
#include "../core/texture.h"
#include "../core/eglc.h"

#include <libakcore/rational.h>
#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakbuffer/avbuffer.h>

using namespace akashi::core;

static constexpr const char* vshader_src = u8R"(
    #version 420 core
    uniform mat4 mvpMatrix;
    uniform ivec2 uv_flip_hv; // [uv_flip_h, uv_flip_v]
    in vec3 vertices;
    in vec2 lumaUvs;
    in vec2 chromaUvs;

    out VS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } vs_out;

    void poly_main(inout vec4 pos);

    vec2 get_uvs(vec2 raw_uvs){
        return vec2(
            uv_flip_hv[0] == 1 ? 1.0 - raw_uvs.x : raw_uvs.x,
            uv_flip_hv[1] == 1 ? 1.0 - raw_uvs.y : raw_uvs.y
        );
    }
    
    void main(void){
        vs_out.vLumaUvs = get_uvs(lumaUvs);
        vs_out.vChromaUvs = get_uvs(chromaUvs);
        vec4 t_vertices = vec4(vertices, 1.0);
        poly_main(t_vertices);
        gl_Position = mvpMatrix * t_vertices;
    }
)";

static constexpr const char* fshader_src = u8R"(
    #version 420 core
    uniform float time;
    uniform float global_time;
    uniform float local_duration;
    uniform float fps;
    uniform vec2 resolution;
    uniform vec2 mesh_size;

    out vec4 fragColor;

    vec3 get_yuv();

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

    void main(void){
        fragColor = to_rgb();
        frag_main(fragColor);
})";

static constexpr const char* color_conv_fshader_sw = u8R"(
    #version 420 core
    uniform sampler2D textureY;
    uniform sampler2D textureCb;
    uniform sampler2D textureCr;

    in GS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } fs_in;

    vec3 get_yuv(){
        return vec3(
            texture(textureY, fs_in.vLumaUvs).r,
            texture(textureCb, fs_in.vChromaUvs).r,
            texture(textureCr, fs_in.vChromaUvs).r
        );
    }

)";

static constexpr const char* color_conv_fshader_vaapi = u8R"(
    #version 420 core
    uniform sampler2D textureY;
    uniform sampler2D textureCb;

    in GS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } fs_in;

    vec3 get_yuv(){
        return vec3(
            texture(textureY, fs_in.vLumaUvs).r,
            texture(textureCb, fs_in.vLumaUvs).r,
            texture(textureCb, fs_in.vLumaUvs).g
        );
    }
)";

static constexpr const char* default_user_pshader_src = u8R"(
    #version 420 core
    uniform float time;
    uniform float global_time;
    uniform float local_duration;
    uniform float fps;
    uniform vec2 resolution;
    uniform vec2 mesh_size;
    void poly_main(inout vec4 position){
    }
)";

static constexpr const char* default_user_fshader_src = u8R"(
    #version 420 core
    uniform float time;
    uniform vec2 resolution;
    uniform vec2 mesh_size;
    void frag_main(inout vec4 _fragColor){
    }
)";

static constexpr const char* default_user_gshader_src = u8R"(
    #version 420 core
    layout (triangles) in;
    layout (triangle_strip, max_vertices = 3) out;

    in VS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } gs_in[];

    out GS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } gs_out;
    
    void main() { 
        for(int i = 0; i < 3; i++){
            gs_out.vLumaUvs = gs_in[i].vLumaUvs; 
            gs_out.vChromaUvs = gs_in[i].vChromaUvs;
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }  
)";

namespace akashi {
    namespace graphics {

        struct VideoActor::Pass : public layer_commons::CommonProgramLocation,
                                  public layer_commons::Transform {
            GLuint prog;
            QuadMesh mesh;
            GLuint texY_loc;
            GLuint texCb_loc;
            GLuint texCr_loc;
            VideoTexture vtex;
        };

        bool VideoActor::create(OGLRenderContext& /*ctx */, const core::LayerContext& layer_ctx) {
            m_layer_ctx = layer_ctx;
            m_layer_type = static_cast<core::LayerType>(layer_ctx.type);

            return true;
        }

        bool VideoActor::render(OGLRenderContext& ctx, const core::Rational& pts,
                                const Camera& camera) {
            if (m_current_pts == pts) {
                AKLOG_INFON("PTS unchanged");
                if (m_pass) {
                    CHECK_AK_ERROR2(this->render_inner(ctx, pts, camera));
                    AKLOG_INFON("Rendering with the last frame");
                }
                return true;
            }

            auto buf_data = ctx.dequeue(m_layer_ctx.uuid + std::to_string(ctx.loop_cnt()), pts);
            if (!buf_data) {
                AKLOG_INFON("Dequeue failed");
                if (m_pass) {
                    CHECK_AK_ERROR2(this->render_inner(ctx, pts, camera));
                    AKLOG_INFON("Rendering with the last frame");
                }
                return true;
            }

            if (!m_pass) {
                m_pass = new VideoActor::Pass;
                CHECK_AK_ERROR2(this->load_pass(ctx, std::move(buf_data)));
            } else {
                CHECK_AK_ERROR2(
                    m_pass->vtex.update(ctx, m_layer_ctx.video_layer_ctx, std::move(buf_data)));
            }

            CHECK_AK_ERROR2(this->render_inner(ctx, pts, camera));
            m_current_pts = pts;
            return true;
        }

        bool VideoActor::destroy(const OGLRenderContext& /*ctx */) {
            if (m_pass) {
                m_pass->mesh.destroy();
                m_pass->vtex.destroy();
                glDeleteProgram(m_pass->prog);
                delete m_pass;
            }
            m_pass = nullptr;
            return true;
        }

        bool VideoActor::render_inner(OGLRenderContext& ctx, const core::Rational& pts,
                                      const Camera& camera) {
            glUseProgram(m_pass->prog);

            m_pass->vtex.use_textures({m_pass->texY_loc, m_pass->texCb_loc, m_pass->texCr_loc});

            glm::mat4 new_mvp = camera.vp_mat() * m_pass->model_mat;
            glUniformMatrix4fv(m_pass->mvp_loc, 1, GL_FALSE, &new_mvp[0][0]);

            auto local_pts = pts - m_layer_ctx.from;
            glUniform1f(m_pass->time_loc, local_pts.to_decimal());
            glUniform1f(m_pass->global_time_loc, pts.to_decimal());

            auto local_duration = m_layer_ctx.to - m_layer_ctx.from;
            glUniform1f(m_pass->local_duration_loc, local_duration.to_decimal());
            glUniform1f(m_pass->fps_loc, ctx.fps().to_decimal());

            auto res = ctx.resolution();
            glUniform2f(m_pass->resolution_loc, res[0], res[1]);

            glBindVertexArray(m_pass->mesh.vao());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pass->mesh.ibo());

            glDrawElements(GL_TRIANGLES, m_pass->mesh.ibo_length(), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            return true;
        }

        bool VideoActor::load_pass(const OGLRenderContext& ctx,
                                   core::owned_ptr<buffer::AVBufferData>&& buf_data) {
            CHECK_AK_ERROR2(
                m_pass->vtex.create(ctx, m_layer_ctx.video_layer_ctx, std::move(buf_data)));

            m_pass->prog = glCreateProgram();

            CHECK_AK_ERROR2(this->load_shaders());

            m_pass->mvp_loc = glGetUniformLocation(m_pass->prog, "mvpMatrix");
            m_pass->texY_loc = glGetUniformLocation(m_pass->prog, "textureY");
            m_pass->texCb_loc = glGetUniformLocation(m_pass->prog, "textureCb");
            m_pass->texCr_loc = glGetUniformLocation(m_pass->prog, "textureCr");
            m_pass->time_loc = glGetUniformLocation(m_pass->prog, "time");
            m_pass->global_time_loc = glGetUniformLocation(m_pass->prog, "global_time");
            m_pass->local_duration_loc = glGetUniformLocation(m_pass->prog, "local_duration");
            m_pass->fps_loc = glGetUniformLocation(m_pass->prog, "fps");
            m_pass->resolution_loc = glGetUniformLocation(m_pass->prog, "resolution");
            m_pass->mesh_size_loc = glGetUniformLocation(m_pass->prog, "mesh_size");

            auto vertices_loc = glGetAttribLocation(m_pass->prog, "vertices");
            auto luma_uvs_loc = glGetAttribLocation(m_pass->prog, "lumaUvs");
            auto chroma_uvs_loc = glGetAttribLocation(m_pass->prog, "chromaUvs");

            std::array<float, 2> mesh_size = layer_commons::get_mesh_size(
                m_layer_ctx, {m_pass->vtex.info().video_width, m_pass->vtex.info().video_height});

            CHECK_AK_ERROR2(m_pass->mesh.create(mesh_size, m_pass->vtex.info(), vertices_loc,
                                                luma_uvs_loc, chroma_uvs_loc));

            m_pass->trans_vec =
                layer_commons::get_trans_vec({m_layer_ctx.x, m_layer_ctx.y, m_layer_ctx.z});
            m_pass->scale_vec = glm::vec3(1.0f) * (float)m_layer_ctx.video_layer_ctx.scale;
            layer_commons::update_model_mat(m_pass, m_layer_ctx);

            {
                glUseProgram(m_pass->prog);
                auto uv_flip_hv_loc = glGetUniformLocation(m_pass->prog, "uv_flip_hv");
                glUniform2i(uv_flip_hv_loc, m_layer_ctx.uv_flip_h, m_layer_ctx.uv_flip_v);
                glUniform2fv(m_pass->mesh_size_loc, 1, m_pass->mesh.mesh_size().data());
                glUseProgram(0);
            }

            return true;
        }

        bool VideoActor::load_shaders() {
            CHECK_AK_ERROR2(compile_attach_shader(m_pass->prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(m_pass->prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(
                m_pass->prog, GL_FRAGMENT_SHADER,
                m_pass->vtex.decode_method() == VideoDecodeMethod::SW ? color_conv_fshader_sw
                                                                      : color_conv_fshader_vaapi));

            auto frag_shader = m_layer_ctx.video_layer_ctx.frag;
            CHECK_AK_ERROR2(compile_attach_shader(m_pass->prog, GL_FRAGMENT_SHADER,
                                                  frag_shader.empty() ? default_user_fshader_src
                                                                      : frag_shader.c_str()));

            auto poly_shader = m_layer_ctx.video_layer_ctx.poly;
            CHECK_AK_ERROR2(compile_attach_shader(m_pass->prog, GL_VERTEX_SHADER,
                                                  poly_shader.empty() ? default_user_pshader_src
                                                                      : poly_shader.c_str()));

            CHECK_AK_ERROR2(
                compile_attach_shader(m_pass->prog, GL_GEOMETRY_SHADER, default_user_gshader_src));

            CHECK_AK_ERROR2(link_shader(m_pass->prog));
            return true;
        }

    }

}
