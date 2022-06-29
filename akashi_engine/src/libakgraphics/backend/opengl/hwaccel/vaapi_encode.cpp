#include "./vaapi_encode.h"

#include "../render_context.h"
#include "../core/texture.h"
#include "../core/eglc.h"
#include "../core/glc.h"
#include "../core/shader.h"

#include <libakcore/error.h>
#include <libakcore/logger.h>
#include <libakbuffer/hwframe_vaapi.h>

#include <libdrm/drm_fourcc.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <unistd.h>

using namespace akashi::core;

static constexpr const char* vshader_src = u8R"(
    #version 420 core
    in vec3 vertices;

    void main(void){
        gl_Position = vec4(vertices, 1.0);
    }
)";

static constexpr const char* fshader_src = u8R"(
    #version 420 core
    #extension GL_ARB_explicit_uniform_location : require

    uniform layout(rgba8, binding=0) readonly image2D u_rgb_img;
    uniform layout(r8, binding=1) writeonly image2D u_y_img;
    uniform layout(rg8, binding=2) writeonly image2D u_uv_img;

    layout(location=4) uniform ivec2 resolution; 

    // BT.709
    vec3 to_yuv_bt709_old(vec3 rgb){
        return vec3(
          (0.2126 * rgb.r) + (0.7152 * rgb.g) + (0.0722 * rgb.b),
          (-0.114572 * rgb.r) + (-0.385428 * rgb.g) + (0.5 * rgb.b) + 0.5,
          (0.5 * rgb.r) + (-0.454153 * rgb.g) + (-0.045847 * rgb.b) + 0.5
        );
    }

    const mat3 mat_yuv_bt709 = mat3(
         0.2126,     0.7152,    0.0722,
        -0.114572,  -0.385428,  0.5,
         0.5,       -0.454153, -0.045847
    );

    vec3 to_yuv_bt709(vec3 rgb){
        // transpose
        return (rgb * mat_yuv_bt709) + vec3(0, 0.5, 0.5);
    }

    // BT.601
    vec3 to_yuv_bt601(vec3 rgb){
        return vec3(
          (0.299 * rgb.r) + (0.587 * rgb.g) + (0.114 * rgb.b),
          (-0.168736 * rgb.r) + (-0.331264 * rgb.g) + (0.5 * rgb.b) + 0.5,
          (0.5 * rgb.r) + (-0.418688 * rgb.g) + (-0.081312 * rgb.b) + 0.5
        );
    }

    // float derivative_avg(float a){
    //     float a00 = a;
    //     float a10 = dFdx(a00) + a00;
    //     float a01 = dFdy(a00) + a00;
    //     float a11 = dFdx(a01) + a01; // NB: we cannot do something like `dFdx(dFdy(a00))`
    //     return (a00 + a10 + a01 + a11) / 4.0;
    // }

    vec2 derivative_avg(vec2 uv){
        vec2 d10 = dFdx(uv);
        vec2 d01 = dFdy(uv);
        vec2 d11 = dFdx(d01); // NB: we cannot do something like `dFdx(dFdy(uv))`
        return ((0 + d10 + d01 + d11) * 0.25) + uv;
    }
      
    vec3 clip_rgb(vec3 rgb){
        return (rgb * 0.8588) + vec3(0.0625, 0.0625, 0.0625);
    }

    void main(void){
         
        ivec2 pos = ivec2(gl_FragCoord.xy);
        
        // flip rgba_tex
        vec3 rgb = clip_rgb(imageLoad(u_rgb_img, ivec2(pos.x, (resolution.y -1) - pos.y)).rgb);
        vec3 yuv = to_yuv_bt709(rgb);

        imageStore(u_y_img, pos, vec4(yuv.x, 0, 0, 0));

        ivec2 uv_pos = pos.x % 2 == 0 && pos.y % 2 == 0 ? ivec2(pos.x/2, pos.y/2) : ivec2(-10,-10);
        vec2 uv = derivative_avg(yuv.yz);
        // Pick-one approach is not bad a choise in some situations
        // vec2 uv = yuv.yz;
        imageStore(u_uv_img, uv_pos, vec4(uv, 0, 0));
    }
)";

namespace akashi {

    namespace buffer {

        struct VAAPIEGLContext {
            VAImage va_image;
            VADRMPRIMESurfaceDescriptor desc;
            EGLImage egl_images[3] = {nullptr, nullptr, nullptr};
        };

        class GFXHWContextImpl : public GFXHWContext {
          public:
            explicit GFXHWContextImpl(const VAAPIEGLContext& vaapi_ctx) : m_vaapi_ctx(vaapi_ctx){};

            virtual ~GFXHWContextImpl() {
                for (uint32_t i = 0; i < m_vaapi_ctx.desc.num_layers; i++) {
                    for (uint32_t j = 0; j < m_vaapi_ctx.desc.layers[i].num_planes; j++) {
                        auto obj_idx = m_vaapi_ctx.desc.layers[i].object_index[j];
                        close(m_vaapi_ctx.desc.objects[obj_idx].fd);
                    }
                }
                for (int i = 0; i < 3; i++) {
                    if (m_vaapi_ctx.egl_images[i]) {
                        akashi::graphics::eglDestroyImageKHR(
                            akashi::graphics::eglGetCurrentDisplay(), m_vaapi_ctx.egl_images[i]);
                    }
                    m_vaapi_ctx.egl_images[i] = nullptr;
                }
            };

          private:
            VAAPIEGLContext m_vaapi_ctx;
        };

    }

    namespace graphics {

        struct VAAPIHWEncodeFBO::Context {
            GLuint fbo;
            GLuint fbo_tex;

            std::array<OGLTexture, 3> yuv_texes;

            GLuint prog;
            GLuint vao;
            GLuint ibo;
            int ibo_length = 0;
        };

        VAAPIHWEncodeFBO::VAAPIHWEncodeFBO(OGLRenderContext& render_ctx) : HWEncodeFBO() {
            m_ctx = new VAAPIHWEncodeFBO::Context();

            this->load_fbo(render_ctx);

            GLuint tex_bufs[3];
            glGenTextures(3, tex_bufs);

            for (int i = 0; i < 3; i++) {
                m_ctx->yuv_texes[i].buffer = tex_bufs[i];
            }

            this->load_pass(render_ctx);
        }

        VAAPIHWEncodeFBO::~VAAPIHWEncodeFBO() {
            if (m_ctx) {
                glDeleteFramebuffers(1, &m_ctx->fbo);
                glDeleteTextures(1, &m_ctx->fbo_tex);

                // better to use free_ogl_texture() instead
                GLuint yuv_tex_bufs[3];
                for (int i = 0; i < 3; i++) {
                    yuv_tex_bufs[i] = m_ctx->yuv_texes[i].buffer;
                }
                glDeleteTextures(3, yuv_tex_bufs);

                glDeleteBuffers(1, &m_ctx->ibo);
                glDeleteVertexArrays(1, &m_ctx->vao);
                glDeleteProgram(m_ctx->prog);

                delete m_ctx;
            }
        }

        void VAAPIHWEncodeFBO::render(OGLRenderContext& render_ctx, GLuint rgba_tex,
                                      core::borrowed_ptr<buffer::HWFrame> hwframe) {
            auto hw_info = hwframe->hw_info();
            buffer::VAAPIEGLContext vaegl_ctx;
            this->load_external_textures(&vaegl_ctx, hw_info);
            hwframe->set_gfx_hwctx(core::make_owned<buffer::GFXHWContextImpl>(vaegl_ctx));

            // render
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->fbo);

                glViewport(0, 0, render_ctx.resolution()[0], render_ctx.resolution()[1]);

                glUseProgram(m_ctx->prog);

                glBindImageTexture(0, rgba_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
                glBindImageTexture(1, m_ctx->yuv_texes[0].buffer, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                                   GL_R8);
                glBindImageTexture(2, m_ctx->yuv_texes[1].buffer, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                                   GL_RG8);

                glBindVertexArray(m_ctx->vao);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ctx->ibo);
                glDrawElements(GL_TRIANGLES, m_ctx->ibo_length, GL_UNSIGNED_SHORT, 0);

                glBindVertexArray(0);

                // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                // glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
                // glMemoryBarrier(GL_ALL_BARRIER_BITS);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            glFlush(); // this should be necessary at the moment

            if (auto status = vaSyncSurface(hw_info.va_display, hw_info.va_surface_id);
                status != 0) {
                AKLOG_ERROR("vaSyncSurface() failed: {}", vaErrorStr(status));
            }
        }

        bool VAAPIHWEncodeFBO::load_fbo(OGLRenderContext& render_ctx) {
            glGenTextures(1, &m_ctx->fbo_tex);

            glBindTexture(GL_TEXTURE_2D, m_ctx->fbo_tex);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, render_ctx.resolution()[0],
                         render_ctx.resolution()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenFramebuffers(1, &m_ctx->fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->fbo);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   m_ctx->fbo_tex, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            auto err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (err != GL_FRAMEBUFFER_COMPLETE) {
                AKLOG_ERROR("Failed to create FBO: 0x{:x}, {}", err, gl_err_to_str(err));
                return false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            return true;
        }

        bool VAAPIHWEncodeFBO::load_pass(OGLRenderContext& render_ctx) {
            m_ctx->prog = glCreateProgram();

            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(m_ctx->prog, GL_FRAGMENT_SHADER, fshader_src));

            CHECK_AK_ERROR2(link_shader(m_ctx->prog));

            this->create_mesh(render_ctx);

            {
                glUseProgram(m_ctx->prog);
                glUniform2i(4, render_ctx.resolution()[0], render_ctx.resolution()[1]);
            }

            return true;
        }

        void VAAPIHWEncodeFBO::create_mesh(OGLRenderContext& /* render_ctx */) {
            glGenVertexArrays(1, &m_ctx->vao);
            glBindVertexArray(m_ctx->vao);

            // vertices
            auto quad_hwidth = 1.0f;
            auto quad_hheight = 1.0f;
            GLfloat vertices[] = {
                -quad_hwidth, quad_hheight,  0.0, // left-top
                quad_hwidth,  quad_hheight,  0.0, // right-top
                -quad_hwidth, -quad_hheight, 0.0, // left-bottom
                quad_hwidth,  -quad_hheight, 0.0  // right-bottom
            };
            GLuint vertices_vbo;
            glGenBuffers(1, &vertices_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            auto vertices_loc = glGetAttribLocation(m_ctx->prog, "vertices");
            glEnableVertexAttribArray(vertices_loc);
            glVertexAttribPointer(vertices_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                                  (GLvoid*)0);

            // ibo
            unsigned short indices[] = {0, 2, 1, 2, 3, 1};
            glGenBuffers(1, &m_ctx->ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ctx->ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            m_ctx->ibo_length = 6;
        }

        bool VAAPIHWEncodeFBO::load_external_textures(buffer::VAAPIEGLContext* vaegl_ctx,
                                                      const buffer::HWFrameInfo& hw_info) {
            if (auto status = vaExportSurfaceHandle(hw_info.va_display, hw_info.va_surface_id,
                                                    VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                                                    VA_EXPORT_SURFACE_WRITE_ONLY |
                                                        VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                                                    &vaegl_ctx->desc);
                status != 0) {
                AKLOG_ERROR("vaExportSurfaceHandle() failed: {}", vaErrorStr(status));
                return false;
            }

            for (uint32_t i = 0; i < vaegl_ctx->desc.num_layers; i++) {
                auto& tex = m_ctx->yuv_texes[i];

                switch (vaegl_ctx->desc.layers[i].drm_format) {
                    case DRM_FORMAT_R8: {
                        tex.format = GL_RED;
                        tex.internal_format = GL_R8;
                        tex.width = vaegl_ctx->desc.width;
                        tex.height = vaegl_ctx->desc.height;
                        tex.effective_width = vaegl_ctx->desc.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    case DRM_FORMAT_RG88: {
                        tex.format = GL_RG;
                        tex.internal_format = GL_RG8;
                        tex.width = vaegl_ctx->desc.width / 2;
                        tex.height = vaegl_ctx->desc.height / 2;
                        tex.effective_width = tex.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    case DRM_FORMAT_GR88: {
                        tex.format = GL_RG;
                        tex.internal_format = GL_RG8;
                        tex.reversed = 1;
                        tex.width = vaegl_ctx->desc.width / 2;
                        tex.height = vaegl_ctx->desc.height / 2;
                        tex.effective_width = tex.width;
                        tex.effective_height = tex.height;
                        break;
                    }
                    default: {
                        AKLOG_ERROR("Not supported drm format found: {}",
                                    vaegl_ctx->desc.layers[i].drm_format);
                        return false;
                    }
                }

                // glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, tex.buffer);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                auto obj_idx = vaegl_ctx->desc.layers[i].object_index[0]; // singleplane layer
                EGLuint64KHR modifier = vaegl_ctx->desc.objects[obj_idx].drm_format_modifier;

                // clang-format off
                EGLint attribs[] = {
                    EGL_WIDTH, tex.width,
                    EGL_HEIGHT, tex.height,
                    EGL_LINUX_DRM_FOURCC_EXT, (EGLint)vaegl_ctx->desc.layers[i].drm_format,
                    EGL_DMA_BUF_PLANE0_FD_EXT, vaegl_ctx->desc.objects[obj_idx].fd,
                    EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)vaegl_ctx->desc.layers[i].offset[0], // singleplane layer
                    EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)vaegl_ctx->desc.layers[i].pitch[0], // singleplane layer
                    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (EGLint)(modifier & 0xffffffff),
                    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (EGLint)(modifier >> 32),
                    EGL_NONE
                };
                // clang-format on

                // clang-format off
                vaegl_ctx->egl_images[i] = eglCreateImageKHR(
                    eglGetCurrentDisplay(),
                    EGL_NO_CONTEXT,
                    EGL_LINUX_DMA_BUF_EXT,
                    nullptr,
                    attribs
                );
                // clang-format on

                if (!vaegl_ctx->egl_images[i]) {
                    AKLOG_ERROR("eglCreateImageKHR() failed: {}", eglGetError());
                    return false;
                }

                glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, vaegl_ctx->egl_images[i]);

                // call it here in order to avoid GL_INVALID_OPERATION emitted from
                // glEGLImageTargetTexture2DOES()
                // glTexStorage2D(GL_TEXTURE_2D, 1, tex.internal_format, tex.width, tex.height);
                // or we can use glTexImage2D instead?
                glTexImage2D(GL_TEXTURE_2D, 0, tex.internal_format, tex.width, tex.height, 0,
                             tex.format, GL_UNSIGNED_BYTE, nullptr);

                glBindTexture(GL_TEXTURE_2D, 0);
            }

            glBindTexture(GL_TEXTURE_2D, 0);

            return true;
        }

    }
}
