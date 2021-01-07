#include "./video_quad.h"

#include "../../gl.h"
#include "../../core/shader.h"
#include "../../core/buffer.h"
#include "../../core/texture.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>
#include <libakcore/element.h>
#include <libakcore/string.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace akashi::core;

// inspired by https://github.com/brion/yuv-canvas/blob/master/shaders/YCbCr.vsh
static constexpr const char* vshader_src = u8R"(
    #version 420 core
    uniform mat4 mvpMatrix;
    uniform float flipY;
    in vec3 vertices;
    in vec2 lumaUvs;
    in vec2 chromaUvs;

    out VS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } vs_out;
    
    void main(void){
        vs_out.vLumaUvs = lumaUvs;
        vs_out.vChromaUvs = chromaUvs;
        gl_Position = mvpMatrix * vec4(vertices * vec3(1, flipY, 1), 1.0);
    }
)";

// inspired by https://github.com/brion/yuv-canvas/blob/master/shaders/YCbCr.fsh
static constexpr const char* fshader_src = u8R"(
    #version 420 core
    uniform sampler2D textureY;
    uniform sampler2D textureCb;
    uniform sampler2D textureCr;
    uniform float time;
    uniform vec2 resolution;

    in GS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } fs_in;

    out vec4 fragColor;

    void frag_main(inout vec4 rv);

    void main(void){

        float fY = texture(textureY, fs_in.vLumaUvs).x;
        float fCb = texture(textureCb, fs_in.vChromaUvs).x;
        float fCr = texture(textureCr, fs_in.vChromaUvs).x;

        float fYmul = fY * 1.1643828125;

        fragColor = vec4(
            fYmul + 1.59602734375 * fCr - 0.87078515625,
            fYmul - 0.39176171875 * fCb - 0.81296875 * fCr + 0.52959375,
            fYmul + 2.017234375   * fCb - 1.081390625,
            1
        );
        frag_main(fragColor);
})";

static constexpr const char* default_user_fshader_src = u8R"(
    #version 420 core
    uniform float time;
    uniform vec2 resolution;
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

        /* --- VideoQuadPass --- */

        bool VideoQuadPass::create(const GLRenderContext& ctx, const VTexSizeFormat& format,
                                   const core::LayerContext& layer) {
            m_prop.prog = GET_GLFUNC(ctx, glCreateProgram)();
            m_size_format = format;

            // loading shader
            m_shader_set.load(layer, core::LayerType::VIDEO);
            this->load_shader(ctx, m_prop.prog, m_shader_set);

            // uniform location
            m_prop.mvp_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "mvpMatrix");
            m_prop.flipY_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "flipY");
            m_prop.texY_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "textureY");
            m_prop.texCb_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "textureCb");
            m_prop.texCr_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "textureCr");
            m_prop.time_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "time");
            m_prop.resolution_loc =
                GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "resolution");

            CHECK_AK_ERROR2(this->load_vao(ctx, format, m_prop.prog, m_prop.vao));
            CHECK_AK_ERROR2(this->load_ibo(ctx, m_prop.ibo));
            m_prop.ibo_length = 6;

            return true;
        }

        bool VideoQuadPass::destroy(const GLRenderContext& ctx) {
            GET_GLFUNC(ctx, glDeleteBuffers)(1, &m_prop.ibo);
            GET_GLFUNC(ctx, glDeleteVertexArrays)(1, &m_prop.vao);
            GET_GLFUNC(ctx, glDeleteProgram)(m_prop.prog);
            return true;
        }

        const VideoQuadPassProp& VideoQuadPass::get_prop(void) const { return m_prop; }

        void VideoQuadPass::shader_reload(const GLRenderContext& ctx,
                                          const core::LayerContext& layer,
                                          const std::vector<const char*>& paths) {
            for (const auto& path : paths) {
                if (m_shader_set.contains(path)) {
                    this->destroy(ctx);
                    this->create(ctx, m_size_format, layer);
                    break;
                }
            }
        }

        bool VideoQuadPass::load_shader(const GLRenderContext& ctx, const GLuint prog,
                                        const UserShaderSet& shader_set) const {
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_FRAGMENT_SHADER,
                                                  shader_set.frag().path.empty()
                                                      ? default_user_fshader_src
                                                      : shader_set.frag().body.c_str()));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_GEOMETRY_SHADER,
                                                  shader_set.geom().path.empty()
                                                      ? default_user_gshader_src
                                                      : shader_set.geom().body.c_str()));
            CHECK_AK_ERROR2(link_shader(ctx, prog));
            return true;
        }

        bool VideoQuadPass::load_vao(const GLRenderContext& ctx, const VTexSizeFormat& format,
                                     const GLuint prog, GLuint& vao) const {
            GLfloat quadWidth = 1.0f;
            GLfloat quadHeight = 1.0f;

            GLfloat vertices[] = {
                -quadWidth, quadHeight,  0.0, // left-top
                quadWidth,  quadHeight,  0.0, // right-top
                -quadWidth, -quadHeight, 0.0, // left-bottom
                quadWidth,  -quadHeight, 0.0  // right-bottom
            };
            GLuint vertices_vbo;
            create_buffer(ctx, vertices_vbo, GL_ARRAY_BUFFER, vertices, sizeof(vertices));

            GLfloat lx0 = 0.0;
            GLfloat ly0 = 1.0;
            // [XXX] since there is a stride, not all of the uploaded textures are used
            GLfloat lx1 = (GLfloat)(format.video_width) / format.luma_tex_width;
            GLfloat ly1 = 0.0;

            GLfloat luma_uvs[] = {
                lx0, ly1, // left-top
                lx1, ly1, // right-top
                lx0, ly0, // left-bottom
                lx1, ly0, // right-bottom
            };
            GLuint lumaUvs_vbo;
            create_buffer(ctx, lumaUvs_vbo, GL_ARRAY_BUFFER, luma_uvs, sizeof(luma_uvs));

            GLfloat cx0 = 0.0;
            GLfloat cy0 = 1.0;
            // [XXX] since there is a stride, not all of the uploaded textures are used
            GLfloat cx1 = (GLfloat)(format.video_width) / format.chroma_tex_width;
            GLfloat cy1 = 0.0;

            GLfloat chroma_uvs[] = {
                cx0, cy1, // left-top
                cx1, cy1, // right-top
                cx0, cy0, // left-bottom
                cx1, cy0, // right-bottom
            };
            GLuint chromaUvs_vbo;
            create_buffer(ctx, chromaUvs_vbo, GL_ARRAY_BUFFER, chroma_uvs, sizeof(chroma_uvs));

            GET_GLFUNC(ctx, glGenVertexArrays)(1, &vao);
            GET_GLFUNC(ctx, glBindVertexArray)(vao);

            // vertices attribute
            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, vertices_vbo);
            // [TODO] error check?
            GLint verticesLoc = GET_GLFUNC(ctx, glGetAttribLocation)(prog, "vertices");
            GET_GLFUNC(ctx, glEnableVertexAttribArray)(verticesLoc);
            GET_GLFUNC(ctx, glVertexAttribPointer)
            (verticesLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

            // lumaUvs attribute
            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, lumaUvs_vbo);
            // [TODO] error check?
            GLint lumaUvsLoc = GET_GLFUNC(ctx, glGetAttribLocation)(prog, "lumaUvs");
            GET_GLFUNC(ctx, glEnableVertexAttribArray)(lumaUvsLoc);
            GET_GLFUNC(ctx, glVertexAttribPointer)
            (lumaUvsLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

            // chromaUvs attribute
            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, chromaUvs_vbo);
            // [TODO] error check?
            GLint chromaUvsLoc = GET_GLFUNC(ctx, glGetAttribLocation)(prog, "chromaUvs");
            GET_GLFUNC(ctx, glEnableVertexAttribArray)(chromaUvsLoc);
            GET_GLFUNC(ctx, glVertexAttribPointer)
            (chromaUvsLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, 0);
            GET_GLFUNC(ctx, glBindVertexArray)(0);

            return true;
        }

        bool VideoQuadPass::load_ibo(const GLRenderContext& ctx, GLuint& ibo) const {
            // [XXX] make sure to use the same type of the values to be passed to glDrawElements
            unsigned short indices[] = {
                0, 1, 2, // left
                1, 2, 3  // right
            };
            create_buffer(ctx, ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
            return true;
        }

        /* --- VideoQuadObject --- */

        void VideoQuadObject::create(const GLRenderContext&, const VideoQuadPass&& pass,
                                     VideoQuadMesh&& mesh) {
            m_prop.pass = std::move(pass);
            m_prop.mesh = mesh;
            m_created = true;
        }

        void VideoQuadObject::destroy(const GLRenderContext& ctx) {
            free_texture(ctx, m_prop.mesh.texY);
            free_texture(ctx, m_prop.mesh.texCb);
            free_texture(ctx, m_prop.mesh.texCr);
        }

        const VideoQuadObjectProp& VideoQuadObject::get_prop(void) const { return m_prop; }

        bool VideoQuadObject::created(void) const { return m_created; }

        void VideoQuadObject::update_pass(const GLRenderContext& ctx, VideoQuadPass&& pass) {
            m_prop.pass.destroy(ctx);
            m_prop.pass = pass;
        }

        void VideoQuadObject::update_mesh(const GLRenderContext& ctx, VideoQuadMesh&& mesh) {
            // [TODO] is it ok to free it here?
            free_texture(ctx, m_prop.mesh.texY);
            free_texture(ctx, m_prop.mesh.texCb);
            free_texture(ctx, m_prop.mesh.texCr);

            m_prop.mesh = mesh;
        }

    }
}
