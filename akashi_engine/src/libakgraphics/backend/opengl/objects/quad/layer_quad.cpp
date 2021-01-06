#include "./layer_quad.h"

#include "../../gl.h"
#include "../../core/shader.h"
#include "../../core/buffer.h"
#include "../../core/texture.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

using namespace akashi::core;

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

static constexpr const char* vshader_src = u8R"(
    #version 420 core
    uniform mat4 mvpMatrix;
    uniform float flipY;
    in vec3 vertices;
    in vec2 uvs;

    out vec2 vUvs;
    
    void main(void){
        vUvs = uvs;
        gl_Position = mvpMatrix * vec4(vertices * vec3(1, flipY, 1), 1.0);
    }
)";

static constexpr const char* fshader_src = u8R"(
    #version 420 core
    uniform sampler2D texture0;
    in vec2 vUvs;

    out vec4 fragColor;

    void frag_main(inout vec4 rv);

    void main(void){
        vec4 smpColor = texture(texture0, vUvs);
        fragColor = smpColor;
        frag_main(fragColor);
    }
)";

static constexpr const char* default_user_fshader_src = u8R"(
    #version 420 core
    uniform float time;
    uniform vec2 resolution;
    void frag_main(inout vec4 _fragColor){
    }
)";

namespace akashi {
    namespace graphics {

        /* --- LayerQuadPass --- */

        static json::optional::type<std::string> get_frag_path(const core::LayerContext& layer,
                                                               const core::LayerType& type) {
            switch (type) {
                case LayerType::TEXT: {
                    return layer.text_layer_ctx.frag_path;
                }
                case LayerType::IMAGE: {
                    return layer.image_layer_ctx.frag_path;
                }
                default: {
                    AKLOG_ERROR("LayerQuadPass is not currently implemented for the type: {}",
                                type);
                    throw std::runtime_error(
                        "LayerQuadPass is not currently implemented for the type");
                }
            }
        }

        bool LayerQuadPass::create(const GLRenderContext& ctx, const core::LayerContext& layer,
                                   const core::LayerType& type) {
            m_prop.prog = GET_GLFUNC(ctx, glCreateProgram)();

            // [TODO] maybe we should return values after execute deleteProgram
            // when link_shader fails
            // [TODO] frag_path lifetime?
            if (!json::optional::is_none(get_frag_path(layer, type)) &&
                !json::optional::unwrap(get_frag_path(layer, type)).empty()) {
                std::ifstream ist(json::optional::unwrap(get_frag_path(layer, type)));
                std::stringstream sbuf;
                sbuf << ist.rdbuf();
                this->load_shader(ctx, m_prop.prog, sbuf.str().c_str());
            } else {
                this->load_shader(ctx, m_prop.prog, default_user_fshader_src);
            }

            // uniform location
            m_prop.mvp_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "mvpMatrix");
            m_prop.flipY_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "flipY");
            m_prop.tex_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "texture0");
            m_prop.time_loc = GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "time");
            m_prop.resolution_loc =
                GET_GLFUNC(ctx, glGetUniformLocation)(m_prop.prog, "resolution");

            CHECK_AK_ERROR2(this->load_vao(ctx, m_prop.prog, m_prop.vao));

            CHECK_AK_ERROR2(this->load_ibo(ctx, m_prop.ibo));
            m_prop.ibo_length = 6;

            return true;
        }

        bool LayerQuadPass::destroy(const GLRenderContext& ctx) {
            GET_GLFUNC(ctx, glDeleteBuffers)(1, &m_prop.ibo);
            GET_GLFUNC(ctx, glDeleteVertexArrays)(1, &m_prop.vao);
            GET_GLFUNC(ctx, glDeleteProgram)(m_prop.prog);
            return true;
        }

        const LayerQuadPassProp& LayerQuadPass::get_prop(void) const { return m_prop; }

        void LayerQuadPass::shader_reload(const GLRenderContext& ctx,
                                          const core::LayerContext& layer,
                                          const core::LayerType& type,
                                          const std::vector<const char*> paths) {
            std::string shader_path{""};
            if (!json::optional::is_none(get_frag_path(layer, type)) &&
                !json::optional::unwrap(get_frag_path(layer, type)).empty()) {
                shader_path = json::optional::unwrap(get_frag_path(layer, type));
            }
            if (shader_path.empty()) {
                return;
            }

            for (const auto& path : paths) {
                if (shader_path == std::string(path)) {
                    this->destroy(ctx);
                    this->create(ctx, layer, type);
                    break;
                }
            }
        }

        bool LayerQuadPass::load_shader(const GLRenderContext& ctx, const GLuint prog,
                                        const char* user_fshader_src) const {
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_VERTEX_SHADER, vshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_FRAGMENT_SHADER, fshader_src));
            CHECK_AK_ERROR2(compile_attach_shader(ctx, prog, GL_FRAGMENT_SHADER, user_fshader_src));
            CHECK_AK_ERROR2(link_shader(ctx, prog));
            return true;
        }

        bool LayerQuadPass::load_vao(const GLRenderContext& ctx, const GLuint prog,
                                     GLuint& vao) const {
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

            GLfloat uvs[] = {
                0.0, 0.0, // left-top
                1.0, 0.0, // right-top
                0.0, 1.0, // left-bottom
                1.0, 1.0, // right-bottom
            };
            GLuint uvs_vbo;
            create_buffer(ctx, uvs_vbo, GL_ARRAY_BUFFER, uvs, sizeof(uvs));

            GET_GLFUNC(ctx, glGenVertexArrays)(1, &vao);
            GET_GLFUNC(ctx, glBindVertexArray)(vao);

            // vertices attribute
            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, vertices_vbo);
            // [TODO] error check?
            GLint verticesLoc = GET_GLFUNC(ctx, glGetAttribLocation)(prog, "vertices");
            GET_GLFUNC(ctx, glEnableVertexAttribArray)(verticesLoc);
            GET_GLFUNC(ctx, glVertexAttribPointer)
            (verticesLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

            // uvs attribute
            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, uvs_vbo);
            // [TODO] error check?
            GLint texCoordLoc = GET_GLFUNC(ctx, glGetAttribLocation)(prog, "uvs");
            GET_GLFUNC(ctx, glEnableVertexAttribArray)(texCoordLoc);
            GET_GLFUNC(ctx, glVertexAttribPointer)
            (texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

            GET_GLFUNC(ctx, glBindBuffer)(GL_ARRAY_BUFFER, 0);
            GET_GLFUNC(ctx, glBindVertexArray)(0);

            return true;
        }

        bool LayerQuadPass::load_ibo(const GLRenderContext& ctx, GLuint& ibo) const {
            // [XXX] make sure to use the same type of the values to be passed to glDrawElements
            unsigned short indices[] = {
                0, 1, 2, // left
                1, 2, 3  // right
            };
            create_buffer(ctx, ibo, GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
            return true;
        }

        /* --- LayerQuadObject --- */

        void LayerQuadObject::create(const GLRenderContext&, const LayerQuadPass&& pass,
                                     LayerQuadMesh&& mesh) {
            m_prop.pass = std::move(pass);
            m_prop.mesh = mesh;
        }

        void LayerQuadObject::destroy(const GLRenderContext& ctx) {
            free_texture(ctx, m_prop.mesh.tex);
        }

        const LayerQuadObjectProp& LayerQuadObject::get_prop(void) const { return m_prop; }

        void LayerQuadObject::update_mesh(const GLRenderContext& ctx, LayerQuadMesh&& mesh) {
            // [TODO] is it ok to free it here?
            free_texture(ctx, m_prop.mesh.tex);
            m_prop.mesh = mesh;
        }

    }
}
