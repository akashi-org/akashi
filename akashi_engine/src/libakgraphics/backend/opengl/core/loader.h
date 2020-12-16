#pragma once

#include <libakcore/error.h>

namespace akashi {
    namespace graphics {

        struct GetProcAddress;
        struct GLRenderContext;

        bool parse_gl_version(GLRenderContext& ctx);

        bool parse_shader(GLRenderContext& ctx);

        bool parse_gl_extensions(const GetProcAddress& get_proc_address, GLRenderContext& ctx);

        ak_error_t load_gl_functions(const GetProcAddress& get_proc_address, GLRenderContext& ctx);

        ak_error_t load_gl_getString(const GetProcAddress& get_proc_address, GLRenderContext& ctx);

    }
}
