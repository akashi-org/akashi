#pragma once

#include "../../context.h"

#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <memory>
#include <array>
#include <vector>

namespace akashi {
    namespace core {
        struct RenderProfile;
        struct FrameContext;
    }
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
    }
    namespace state {
        class AKState;
    }
    namespace graphics {

        struct GLRenderContext;
        struct GetProcAddress;
        struct EGLGetProcAddress;
        struct RenderParams;
        struct EncodeRenderParams;
        class OGLRenderContext;
        class Stage;
        class OGLGraphicsContext : public GraphicsContext {
          public:
            explicit OGLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                        core::borrowed_ptr<buffer::AVBuffer> buffer);
            virtual ~OGLGraphicsContext();

            bool load_api(const GetProcAddress& get_proc_address,
                          const EGLGetProcAddress& egl_get_proc_address) override;

            bool load_fbo(const core::RenderProfile& render_prof, bool flip_y = true) override;

            void render(const RenderParams& params, const core::FrameContext& frame_ctx) override;

            void encode_render(EncodeRenderParams& params,
                               const core::FrameContext& frame_ctx) override;

          private:
            core::owned_ptr<OGLRenderContext> m_ogl_ctx;
            core::owned_ptr<Stage> m_stage;
        };

    }
}
