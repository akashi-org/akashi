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
        struct EncodeRenderParams;
        class GraphicsContext;
        class GraphicsContext {
          public:
            explicit GraphicsContext(core::borrowed_ptr<state::AKState>,
                                     core::borrowed_ptr<buffer::AVBuffer>){};
            virtual ~GraphicsContext(){};

            virtual bool load_api(const GetProcAddress& get_proc_address,
                                  const EGLGetProcAddress& egl_get_proc_address) = 0;
            virtual void render(const RenderParams& params,
                                const core::FrameContext& frame_ctx) = 0;
            virtual void encode_render(EncodeRenderParams& params,
                                       const core::FrameContext& frame_ctx) = 0;
        };

    }
}
