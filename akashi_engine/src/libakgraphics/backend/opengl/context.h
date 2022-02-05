#pragma once

#include "../../context.h"
#include "../../osc.h"

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

            void render(const RenderParams& params, const core::FrameContext& frame_ctx) override;

            void encode_render(EncodeRenderParams& params,
                               const core::FrameContext& frame_ctx) override;

          private:
            core::owned_ptr<OGLRenderContext> m_render_ctx;
            core::owned_ptr<Stage> m_stage;
        };

        class OSCRoot;

        class OGLOSCContext : public OSCContext {
          public:
            explicit OGLOSCContext(core::borrowed_ptr<state::AKState>);

            virtual ~OGLOSCContext();

            virtual bool load_api(OSCEventCallback evt_cb, const RenderParams& params,
                                  const GetProcAddress& get_proc_address,
                                  const EGLGetProcAddress& egl_get_proc_address) override;

            virtual void render(const RenderParams& params) override;

            virtual void resize(const RenderParams& params) override;

            virtual bool emit_mouse_event(const OSCMouseEvent& event) override;

            virtual bool emit_time_event(const OSCTimeEvent& event) override;

          private:
            core::owned_ptr<OSCRoot> m_root;
            core::borrowed_ptr<state::AKState> m_state;
        };

    }
}
