#pragma once

#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>

#include <functional>

namespace akashi {
    namespace event {

        enum class EventName {
            DUMMY = -1,
            TIME_UPDATE = 0,
            SET_RENDER_PROF,
            UPDATE,
            CHANGE_PLAY_STATE,
            SEEK_COMPLETED,
        };

        struct Event {
            EventName name = EventName::DUMMY;
            double double_value;
            int64_t int64_value;
            int64_t int64_value2;
            bool bool_value = false;
            core::RenderProfile render_prof;
            state::PlayState play_state;
        };

        struct EventCallback {
            std::function<void(void*, Event)> cb;
        };

        class AKEvent {
          public:
            explicit AKEvent(){};
            virtual ~AKEvent(){};

            virtual void emit_time_update(const core::Rational& time) = 0;

            virtual void emit_set_render_prof(const core::RenderProfile& render_prof) = 0;

            virtual void emit_update(void) = 0;

            virtual void emit_change_play_state(const state::PlayState& play_state) = 0;

            virtual void emit_seek_completed(void) = 0;
        };
    }
}
