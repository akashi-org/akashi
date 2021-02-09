#pragma once

#include <libakcore/memory.h>

namespace akashi {
    namespace core {
        struct RenderProfile;
        struct FrameContext;
        class Rational;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace state {
        class AKState;
    }
    namespace graphics {

        struct GetProcAddress;
        struct EGLGetProcAddress;
        struct RenderParams;
        class GraphicsContext;
        class AKGraphics {
          public:
            explicit AKGraphics(core::borrowed_ptr<state::AKState> state,
                                core::borrowed_ptr<buffer::AVBuffer> buffer);
            virtual ~AKGraphics();

            bool load_api(const GetProcAddress& get_proc_address,
                          const EGLGetProcAddress& egl_get_proc_address);

            bool load_fbo(const core::RenderProfile& render_prof);

            void render(const RenderParams& params, const core::FrameContext& frame_ctx);

          private:
            core::owned_ptr<GraphicsContext> m_gfx_ctx;
        };

    }

}
