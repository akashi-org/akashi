#pragma once

#include "../../../osc.h"

#include <libakcore/memory.h>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace graphics {

        class OSCRenderContext;
        struct RenderParams;
        struct OSCMouseEvent;

        class OSCRoot final {
          public:
            struct Context;

          public:
            explicit OSCRoot(OSCEventCallback evt_cb, const RenderParams& params,
                             core::borrowed_ptr<state::AKState> state);

            virtual ~OSCRoot();

            bool update(const RenderParams& params);

            bool render(const RenderParams& params);

            bool resize(const RenderParams& params);

            bool on_mouse_event(const OSCMouseEvent& event);

            bool on_time_event(const OSCTimeEvent& event);

          private:
            void initialize(const RenderParams& params);

          private:
            core::owned_ptr<OSCRenderContext> m_render_ctx;
            OSCRoot::Context* m_ctx = nullptr;
        };

    }
}
