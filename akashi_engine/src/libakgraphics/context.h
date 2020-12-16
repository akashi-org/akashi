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
    namespace audio {
        class AKAudio;
    }
    namespace state {
        class AKState;
    }
    namespace graphics {

        struct GetProcAddress;
        struct RenderParams;
        class GraphicsContext;
        class GraphicsContext {
          public:
            explicit GraphicsContext(core::borrowed_ptr<state::AKState>,
                                     core::borrowed_ptr<buffer::AVBuffer>,
                                     core::borrowed_ptr<audio::AKAudio>){};
            virtual ~GraphicsContext(){};

            virtual bool load_api(const GetProcAddress& get_proc_address) = 0;
            virtual bool load_fbo(const core::RenderProfile& render_prof) = 0;
            virtual void render(const RenderParams& params,
                                const core::FrameContext& frame_ctx) = 0;
        };

    }
}
