#include "./video_quad.h"

#include "../../gl.h"
#include "../../core/shader.h"
#include "../../core/buffer.h"
#include "../../core/texture.h"
#include "../../../../drm_fourcc_excerpt.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>
#include <libakcore/element.h>
#include <libakcore/string.h>
#include <libakbuffer/avbuffer.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <unistd.h>

#include <va/va.h>
#include <va/va_drmcommon.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

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

static constexpr const char* fshader_src = u8R"(
    #version 420 core
    uniform sampler2D textureY;
    uniform sampler2D textureCb;
    uniform sampler2D textureCr;
    uniform float time;
    uniform float global_time;
    uniform float local_duration;
    uniform vec2 resolution;

    in GS_OUT {
        vec2 vLumaUvs;
        vec2 vChromaUvs;
    } fs_in;

    out vec4 fragColor;

    void to_rgb(inout vec4 _fragColor, vec2 vLumaUvs, vec2 vChromaUvs);

    void frag_main(inout vec4 rv);

    void main(void){
        to_rgb(fragColor, fs_in.vLumaUvs, fs_in.vChromaUvs);
        frag_main(fragColor);
})";

// inspired by https://github.com/brion/yuv-canvas/blob/master/shaders/YCbCr.fsh
static constexpr const char* color_conv_fshader_sw = u8R"(
    #version 420 core
    uniform sampler2D textureY;
    uniform sampler2D textureCb;
    uniform sampler2D textureCr;

    void to_rgb(inout vec4 _fragColor, vec2 vLumaUvs, vec2 vChromaUvs){

        float fY = texture(textureY, vLumaUvs).r;
        float fCb = texture(textureCb, vChromaUvs).r;
        float fCr = texture(textureCr, vChromaUvs).r;

        float fYmul = fY * 1.1643828125;

        _fragColor = vec4(
            fYmul + 1.59602734375 * fCr - 0.87078515625,
            fYmul - 0.39176171875 * fCb - 0.81296875 * fCr + 0.52959375,
            fYmul + 2.017234375   * fCb - 1.081390625,
            1
        );
    }
)";

static constexpr const char* color_conv_fshader_vaapi = u8R"(
    #version 420 core
    uniform sampler2D textureY;
    uniform sampler2D textureCb;

    void to_rgb(inout vec4 _fragColor, vec2 vLumaUvs, vec2 vChromaUvs){

        vec3 rc = vec3(1.164, 0.0, 1.596);
        vec3 gc = vec3(1.164, -0.391, -0.813);
        vec3 bc = vec3(1.164, 2.018, 0.0);

        vec3 offset = vec3(-0.0625, -0.5, -0.5);

        vec3 yuv = vec3(
            texture(textureY, vLumaUvs).r,
            texture(textureCb, vLumaUvs).r,
            texture(textureCb, vLumaUvs).g
        );
        // yuv = clamp(yuv, 0.0, 1.0);
        yuv += offset;

        _fragColor = vec4(
            dot(yuv, rc),
            dot(yuv, gc),
            dot(yuv, bc),
            1
        );
    }
)";

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
                                   const core::LayerContext& layer,
                                   const core::VideoDecodeMethod& decode_method) {
            m_prop.prog = GET_GLFUNC(ctx, glCreateProgram)();
            m_size_format = format;
            m_decode_method = decode_method;

            // loading shader
            m_shader_set.load(layer, core::LayerType::VIDEO);
            this->load_shader(ctx, m_prop.prog, m_shader_set, decode_method);

            // uniform location
            m_prop.mvp_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "mvpMatrix");
            m_prop.flipY_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "flipY");
            m_prop.texY_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "textureY");
            m_prop.texCb_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "textureCb");
            m_prop.texCr_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "textureCr");
            m_prop.time_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "time");
            m_prop.global_time_loc =
                GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "global_time");
            m_prop.local_duration_loc =
                GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "local_duration");
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
                    this->create(ctx, m_size_format, layer, m_decode_method);
                    break;
                }
            }
        }

        bool VideoQuadPass::load_shader(const GLRenderContext& ctx, const GLuint prog,
                                        const UserShaderSet& shader_set,
                                        const core::VideoDecodeMethod& decode_method) const {
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_FRAGMENT_SHADER,
                                                  decode_method == VideoDecodeMethod::SW
                                                      ? color_conv_fshader_sw
                                                      : color_conv_fshader_vaapi));
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

        /* --- VideoQuadMesh --- */

        template <enum core::VideoDecodeMethod>
        bool VideoQuadMesh::create_inner(const GLRenderContext&, const buffer::AVBufferData&) {
            throw std::runtime_error("Not Implemented Error: create_inner()");
        }

        template <>
        bool VideoQuadMesh::create_inner<core::VideoDecodeMethod::SW>(
            const GLRenderContext& ctx, const buffer::AVBufferData& buf_data) {
            for (size_t i = 0; i < m_textures.size(); i++) {
                auto& tex = m_textures[i];
                tex.image = buf_data.prop().video_data[i].buf;
                tex.width = buf_data.prop().video_data[i].stride - 1;
                tex.height = i == 0 ? buf_data.prop().height : buf_data.prop().chroma_height;
                tex.effective_width = i == 0 ? buf_data.prop().width : buf_data.prop().chroma_width;
                tex.effective_height = tex.height;

                tex.format = GL_RED;
                tex.internal_format = GL_R8;

                // [TODO] could this be duplicate with the other indices
                tex.index = i;

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
                GET_GLFUNC(ctx, glTexImage2D)
                (GL_TEXTURE_2D, 0, tex.internal_format, tex.width, tex.height, 0, tex.format,
                 GL_UNSIGNED_BYTE, tex.image);

                // [XXX] make sure to explicity setup when not using mimap
                GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                GET_GLFUNC(ctx, glTexParameteri)
                (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                GET_GLFUNC(ctx, glTexParameteri)
                (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, 0);
            }
            return true;
        }

        template <>
        bool VideoQuadMesh::create_inner<core::VideoDecodeMethod::VAAPI>(
            const GLRenderContext& ctx, const buffer::AVBufferData& buf_data) {
            if (auto status = vaExportSurfaceHandle(
                    buf_data.prop().va_display, buf_data.prop().va_surface_id,
                    VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                    VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                    &m_vaapi_ctx.desc);
                status != 0) {
                AKLOG_ERROR("vaExportSurfaceHandle() failed: {}", vaErrorStr(status));
                return false;
            }

            if (auto status =
                    vaSyncSurface(buf_data.prop().va_display, buf_data.prop().va_surface_id);
                status != 0) {
                AKLOG_ERROR("vaSyncSurface() failed: {}", vaErrorStr(status));
                return false;
            }
            m_vaapi_ctx.need_free = true;

            for (uint32_t i = 0; i < m_vaapi_ctx.desc.num_layers; i++) {
                if (m_vaapi_ctx.desc.layers[i].num_planes > 1) {
                    AKLOG_ERRORN("multiplane layer is not supported");
                    return false;
                }

                auto& tex = m_textures[i];
                tex.index = i;

                switch (m_vaapi_ctx.desc.layers[i].drm_format) {
                    case DRM_FORMAT_R8: {
                        tex.format = GL_RED;
                        tex.internal_format = GL_R8;
                        tex.width = m_vaapi_ctx.desc.width;
                        tex.height = m_vaapi_ctx.desc.height;
                        tex.effective_width = buf_data.prop().width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    case DRM_FORMAT_RG88: {
                        tex.format = GL_RG;
                        tex.internal_format = GL_RG8;
                        tex.width = buf_data.prop().chroma_width;
                        tex.height = buf_data.prop().chroma_height;
                        tex.effective_width = tex.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    case DRM_FORMAT_GR88: {
                        tex.format = GL_RG;
                        tex.internal_format = GL_RG8;
                        tex.reversed = 1;
                        tex.width = buf_data.prop().chroma_width;
                        tex.height = buf_data.prop().chroma_height;
                        tex.effective_width = tex.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    default: {
                        AKLOG_ERROR("not supported drm format found: {}",
                                    m_vaapi_ctx.desc.layers[i].drm_format);
                        return false;
                    }
                }

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
                GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                GET_GLFUNC(ctx, glTexParameteri)
                (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                GET_GLFUNC(ctx, glTexParameteri)
                (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, 0);

                auto obj_idx = m_vaapi_ctx.desc.layers[i].object_index[0]; // singleplane layer
                EGLuint64KHR modifier = m_vaapi_ctx.desc.objects[obj_idx].drm_format_modifier;

                // clang-format off
                EGLint attribs[] = {
                    EGL_WIDTH, tex.width,
                    EGL_HEIGHT, tex.height,
                    EGL_LINUX_DRM_FOURCC_EXT, (EGLint)m_vaapi_ctx.desc.layers[i].drm_format,
                    EGL_DMA_BUF_PLANE0_FD_EXT, m_vaapi_ctx.desc.objects[obj_idx].fd,
                    EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)m_vaapi_ctx.desc.layers[i].offset[0], // singleplane layer
                    EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)m_vaapi_ctx.desc.layers[i].pitch[0], // singleplane layer
                    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(modifier & 0xffffffff),
                    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(modifier >> 32),
                    EGL_NONE
                };
                // clang-format on

                // clang-format off
                m_vaapi_ctx.egl_images[i] = GET_EGLFUNC(ctx, eglCreateImageKHR)(
                    GET_EGLFUNC(ctx, eglGetCurrentDisplay)(),
                    EGL_NO_CONTEXT,
                    EGL_LINUX_DMA_BUF_EXT,
                    nullptr,
                    attribs
                );
                // clang-format on

                if (!m_vaapi_ctx.egl_images[i]) {
                    AKLOG_ERROR("eglCreateImageKHR() failed: {}", GET_EGLFUNC(ctx, eglGetError)());
                    return false;
                }

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
                GET_EGLFUNC(ctx, glEGLImageTargetTexture2DOES)
                (GL_TEXTURE_2D, m_vaapi_ctx.egl_images[i]);
            }

            GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, 0);
            return true;
        }

        template <>
        bool VideoQuadMesh::create_inner<core::VideoDecodeMethod::VAAPI_COPY>(
            const GLRenderContext& ctx, const buffer::AVBufferData& buf_data) {
            for (size_t i = 0; i < 2; i++) {
                auto& tex = m_textures[i];
                tex.image = buf_data.prop().video_data[i].buf;
                tex.width = i == 0 ? buf_data.prop().video_data[i].stride - 1
                                   : buf_data.prop().chroma_width;
                tex.height = i == 0 ? buf_data.prop().height : buf_data.prop().chroma_height;
                tex.effective_width = i == 0 ? buf_data.prop().width : buf_data.prop().chroma_width;
                tex.effective_height = tex.height;

                tex.format = i == 0 ? GL_RED : GL_RG;
                tex.internal_format = i == 0 ? GL_R8 : GL_RG8;

                // [TODO] could this be duplicate with the other indices
                tex.index = i;

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, tex.buffer);
                GET_GLFUNC(ctx, glTexImage2D)
                (GL_TEXTURE_2D, 0, tex.internal_format, tex.width, tex.height, 0, tex.format,
                 GL_UNSIGNED_BYTE, tex.image);

                // [XXX] make sure to explicity setup when not using mimap
                GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                GET_GLFUNC(ctx, glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                GET_GLFUNC(ctx, glTexParameteri)
                (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                GET_GLFUNC(ctx, glTexParameteri)
                (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                GET_GLFUNC(ctx, glBindTexture)(GL_TEXTURE_2D, 0);
            }
            return true;
        }

        bool VideoQuadMesh::create(const GLRenderContext& ctx,
                                   core::owned_ptr<buffer::AVBufferData> buf_data) {
            m_flip_y = ctx.layer_flip_y;
            m_buf_data = std::move(buf_data);
            m_textures.reserve(3);
            m_textures.resize(3);
            for (auto&& tex : m_textures) {
                GET_GLFUNC(ctx, glGenTextures)(1, &tex.buffer);
            }

            m_decode_method = m_buf_data->prop().decode_method;

            switch (m_decode_method) {
                case VideoDecodeMethod::SW: {
                    return this->create_inner<core::VideoDecodeMethod::SW>(ctx, *m_buf_data);
                }
                case VideoDecodeMethod::VAAPI: {
                    return this->create_inner<core::VideoDecodeMethod::VAAPI>(ctx, *m_buf_data);
                }
                case VideoDecodeMethod::VAAPI_COPY: {
                    return this->create_inner<core::VideoDecodeMethod::VAAPI_COPY>(ctx,
                                                                                   *m_buf_data);
                }
                default: {
                    return false;
                }
            }
        }

        bool VideoQuadMesh::destroy(const GLRenderContext& ctx) {
            for (auto&& tex : m_textures) {
                free_texture(ctx, tex);
            }
            if (m_buf_data) {
                m_buf_data.reset(nullptr);
            }
            return true;
        }

        void VideoQuadMesh::update(const GLRenderContext& ctx,
                                   core::owned_ptr<buffer::AVBufferData> buf_data) {
            if (m_decode_method == VideoDecodeMethod::VAAPI) {
                // [XXX] must be called before updating m_buf_data
                this->free_vaapi_context(ctx);
            }
            m_buf_data = std::move(buf_data);
            switch (m_decode_method) {
                case VideoDecodeMethod::SW: {
                    this->create_inner<core::VideoDecodeMethod::SW>(ctx, *m_buf_data);
                    break;
                }
                case VideoDecodeMethod::VAAPI: {
                    this->create_inner<core::VideoDecodeMethod::VAAPI>(ctx, *m_buf_data);
                    break;
                }
                case VideoDecodeMethod::VAAPI_COPY: {
                    this->create_inner<core::VideoDecodeMethod::VAAPI_COPY>(ctx, *m_buf_data);
                    break;
                }
                default: {
                }
            }
        }

        void VideoQuadMesh::free_vaapi_context(const GLRenderContext& ctx) {
            if (m_vaapi_ctx.need_free) {
                for (uint32_t i = 0; i < m_vaapi_ctx.desc.num_layers; i++) {
                    auto obj_idx = m_vaapi_ctx.desc.layers[i].object_index[0]; // singleplane layer
                    close(m_vaapi_ctx.desc.objects[obj_idx].fd);
                }
                for (int i = 0; i < 3; i++) {
                    if (m_vaapi_ctx.egl_images[i]) {
                        GET_EGLFUNC(ctx, eglDestroyImageKHR)
                        (GET_EGLFUNC(ctx, eglGetCurrentDisplay)(), m_vaapi_ctx.egl_images[i]);
                    }
                    m_vaapi_ctx.egl_images[i] = nullptr;
                }
                m_vaapi_ctx.need_free = false;
            }
        }

        /* --- VideoQuadObject --- */

        void VideoQuadObject::create(const GLRenderContext& ctx, const VideoQuadPass&& pass,
                                     core::owned_ptr<buffer::AVBufferData> buf_data) {
            m_prop.pass = std::move(pass);
            m_prop.mesh.create(ctx, std::move(buf_data));
            m_created = true;
        }

        void VideoQuadObject::destroy(const GLRenderContext& ctx) { m_prop.mesh.destroy(ctx); }

        const VideoQuadObjectProp& VideoQuadObject::get_prop(void) const { return m_prop; }

        bool VideoQuadObject::created(void) const { return m_created; }

        void VideoQuadObject::update_pass(const GLRenderContext& ctx, VideoQuadPass&& pass) {
            m_prop.pass.destroy(ctx);
            m_prop.pass = pass;
        }

        void VideoQuadObject::update_mesh(const GLRenderContext& ctx,
                                          core::owned_ptr<buffer::AVBufferData> buf_data) {
            m_prop.mesh.update(ctx, std::move(buf_data));
        }

    }
}
