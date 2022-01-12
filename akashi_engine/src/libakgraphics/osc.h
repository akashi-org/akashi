#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <functional>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace graphics {

        enum class OSCMouseEventKind { PRESS = 0, MOVE, HOLD, RELEASE, LENGTH };
        enum class OSCMouseButton { LEFT = 0, MIDDLE, RIGHT, LENGTH };
        struct OSCMouseEvent {
            OSCMouseEventKind kind = OSCMouseEventKind::LENGTH;
            std::array<int, 2> pos = {-1, -1};
            OSCMouseButton btn = OSCMouseButton::LENGTH;
        };

        enum class OSCTimeEventKind { CURRENT_TIME = 0, DURATION, LENGTH };
        struct OSCTimeEvent {
            OSCTimeEventKind kind = OSCTimeEventKind::LENGTH;
            core::Rational current_time{-1, 1};
            core::Rational duration{-1, 1};
        };

        enum class OSCInnerEventName {
            PLAYBTN_CLICKED = 0,
            VOLUME_CHANGED,
            UPDATE,
            FRAME_SEEK,
            SEEKBAR_PRESSED,
            SEEKBAR_MOVED,
            SEEKBAR_RELEASED,
            LENGTH
        };

        struct OSCInnerEvent {
            OSCInnerEventName name = OSCInnerEventName::LENGTH;
            std::shared_ptr<void> args = nullptr;
        };

        struct OSCEventCallback {
            std::function<void(void*, OSCInnerEvent)> cb;
            void* evt_ctx;
        };

        struct GetProcAddress;
        struct EGLGetProcAddress;
        struct RenderParams;

        class OSCContext {
          public:
            explicit OSCContext(core::borrowed_ptr<state::AKState>){};
            virtual ~OSCContext(){};

            virtual bool load_api(OSCEventCallback evt_cb, const RenderParams& params,
                                  const GetProcAddress& get_proc_address,
                                  const EGLGetProcAddress& egl_get_proc_address) = 0;
            virtual void render(const RenderParams& params) = 0;

            virtual bool emit_mouse_event(const OSCMouseEvent& event) = 0;

            virtual bool emit_time_event(const OSCTimeEvent& event) = 0;
        };

        class OSCWidget {
          public:
            explicit OSCWidget(core::borrowed_ptr<state::AKState> state);

            virtual ~OSCWidget();

            bool load_api(OSCEventCallback evt_cb, const RenderParams& params,
                          const GetProcAddress& get_proc_address,
                          const EGLGetProcAddress& egl_get_proc_address);

            void render(const RenderParams& params);

            bool emit_mouse_event(const OSCMouseEvent& event);

            bool emit_time_event(const OSCTimeEvent& event);

          private:
            core::owned_ptr<OSCContext> m_osc_ctx;
        };

    }

}
