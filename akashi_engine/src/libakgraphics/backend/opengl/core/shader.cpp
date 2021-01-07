#include "./shader.h"

#include "../gl.h"

#include <libakcore/logger.h>

#include <string>
#include <fstream>
#include <sstream>

using namespace akashi::core;

#define GET_SHADER_PATH(shader_type, layer, layer_type)                                            \
    [](const akashi::core::LayerContext& layer_, const akashi::core::LayerType& type_) {           \
        switch (type_) {                                                                           \
            case LayerType::VIDEO: {                                                               \
                return layer_.video_layer_ctx.shader_type##_path;                                  \
            }                                                                                      \
            case LayerType::TEXT: {                                                                \
                return layer_.text_layer_ctx.shader_type##_path;                                   \
            }                                                                                      \
            case LayerType::IMAGE: {                                                               \
                return layer_.image_layer_ctx.shader_type##_path;                                  \
            }                                                                                      \
            default: {                                                                             \
                AKLOG_ERROR("Not implemented Error for the type: {}", type_);                      \
                throw std::runtime_error("Not implemented Error");                                 \
            }                                                                                      \
        }                                                                                          \
    }((layer), (layer_type))

namespace akashi {
    namespace graphics {

        bool compile_attach_shader(const GLRenderContext& ctx, const GLuint prog, const GLenum type,
                                   const char* source) {
            bool res = true;
            GLuint shader = 0;
            GLint compile_status = 0;
            GLint log_length = 0;
            GLchar* log_str = nullptr;

            shader = GET_GLFUNC(ctx, glCreateShader)(type);
            GET_GLFUNC(ctx, glShaderSource)(shader, 1, &source, nullptr);
            GET_GLFUNC(ctx, glCompileShader)(shader);

            GET_GLFUNC(ctx, glGetShaderiv)(shader, GL_COMPILE_STATUS, &compile_status);
            GET_GLFUNC(ctx, glGetShaderiv)(shader, GL_INFO_LOG_LENGTH, &log_length);

            if (log_length > 1) {
                log_str = static_cast<GLchar*>(calloc(log_length + 1, sizeof(GLchar)));
                GET_GLFUNC(ctx, glGetShaderInfoLog)(shader, log_length, nullptr, log_str);
                AKLOG_INFO("Shader compile log: {}", log_str);
            }

            if (!compile_status) {
                AKLOG_ERROR("compile_attach_shader()<type={}> failed: Failed to compile shader: {}",
                            type, compile_status);
                res = false;
                goto exit;
            }

            GET_GLFUNC(ctx, glAttachShader)(prog, shader);

        exit:
            if (log_str != nullptr) {
                free(log_str);
                log_str = nullptr;
            }
            GET_GLFUNC(ctx, glDeleteShader)(shader);
            return res;
        }

        bool link_shader(const GLRenderContext& ctx, const GLuint prog) {
            GLint link_status = 0;
            GLint log_length = 0;

            GET_GLFUNC(ctx, glLinkProgram)(prog);

            GET_GLFUNC(ctx, glGetProgramiv)(prog, GL_LINK_STATUS, &link_status);
            GET_GLFUNC(ctx, glGetProgramiv)(prog, GL_INFO_LOG_LENGTH, &log_length);

            if (!link_status) {
                GLchar* log_str = static_cast<GLchar*>(calloc(log_length + 1, sizeof(GLchar)));
                GET_GLFUNC(ctx, glGetProgramInfoLog)(prog, log_length, nullptr, log_str);
                AKLOG_ERROR("link_program() failed: Failed to link shader: {}, {}", link_status,
                            log_str);
                free(log_str);
                return false;
            } else {
                return true;
            }
        }

        namespace detail {

            template <GLenum shader_type>
            core::json::optional::type<std::string>
            get_shader_path(const akashi::core::LayerContext& layer,
                            const akashi::core::LayerType& layer_type) {
                switch (shader_type) {
                    case GL_FRAGMENT_SHADER: {
                        return GET_SHADER_PATH(frag, layer, layer_type);
                    }
                    case GL_GEOMETRY_SHADER: {
                        return GET_SHADER_PATH(geom, layer, layer_type);
                    }
                    default: {
                        return core::json::optional::none<std::string>;
                    }
                }
            }

        }

        void UserShaderSet::load(const core::LayerContext& layer, const core::LayerType& type) {
            this->read_shader<GL_FRAGMENT_SHADER>(m_frag, layer, type);
            this->read_shader<GL_GEOMETRY_SHADER>(m_geom, layer, type);
        }

        UserShaderSet::~UserShaderSet(void) {}

        bool UserShaderSet::contains(const std::string& path) {
            return path == m_frag.path || path == m_geom.path;
        }

        template <GLenum shader_type>
        void UserShaderSet::read_shader(UserShader& user_shader, const core::LayerContext& layer,
                                        const core::LayerType& layer_type) {
            auto shader_path_opt = detail::get_shader_path<shader_type>(layer, layer_type);
            if (!json::optional::is_none(shader_path_opt)) {
                user_shader.path =
                    json::optional::unwrap(detail::get_shader_path<shader_type>(layer, layer_type));
                if (!user_shader.path.empty()) {
                    std::ifstream ist(user_shader.path);
                    std::stringstream sbuf;
                    sbuf << ist.rdbuf();
                    // [XXX] std::stringstream::str() returns a copy of the underlying
                    // string object
                    user_shader.body = sbuf.str();
                }
            }
        }

    }
}
