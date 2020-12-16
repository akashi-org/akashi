#include "./event.h"

#include "./eval_buffer.h"
#include "./loop/event_loop.h"

#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakeval/akeval.h>
#include <libakwatch/item.h>
#include <libakstate/akstate.h>
#include <libakevent/akevent.h>

#include <functional>

using namespace akashi::core;

namespace akashi {
    namespace player {

        PlayerEvent::PlayerEvent(const event::EventCallback& evt_cb, void* evt_ctx) {
            m_evt_prop.evt_cb = evt_cb;
            m_evt_prop.evt_ctx = evt_ctx;
            m_evt_loop = make_owned<EventLoop>();
        };

        PlayerEvent::~PlayerEvent() = default;

        void PlayerEvent::run(PlayerEventContext ctx) {
            m_evt_loop->run({ctx.state, borrowed_ptr(this), ctx.eval_buf, ctx.buffer});
        }

        void PlayerEvent::emit_pull_render_profile(void) {
            InnerEvent evt;
            evt.name = InnerEventName::PULL_RENDER_PROFILE;
            m_evt_loop->emit_inner_event(evt);
        }

        void PlayerEvent::emit_pull_eval_buffer(size_t length) {
            InnerEvent evt;
            evt.name = InnerEventName::PULL_EVAL_BUFFER;

            auto length_ = static_cast<size_t*>(malloc(sizeof(size_t)));
            if (!length_) {
                AKLOG_ERRORN("PlayerEvent::emit_pull_eval_buffer(): Failed to alloc event ctx");
                return;
            }
            *length_ = length;
            evt.ctx = static_cast<void*>(length_);
            m_evt_loop->emit_inner_event(evt);
        }

        void PlayerEvent::emit_seek(const core::Rational& seek_time) {
            InnerEvent evt;
            evt.name = InnerEventName::SEEK;

            auto seek_time_ = static_cast<Rational*>(malloc(sizeof(Rational)));
            if (!seek_time_) {
                AKLOG_ERRORN("PlayerEvent::emit_seek(): Failed to alloc event ctx");
                return;
            }
            *seek_time_ = seek_time;
            evt.ctx = static_cast<void*>(seek_time_);
            m_evt_loop->emit_inner_event(evt);
        }

        void PlayerEvent::emit_hot_reload(const watch::WatchEventList& event_list) {
            InnerEvent evt;
            evt.name = InnerEventName::HOT_RELOAD;

            auto event_list_ =
                static_cast<watch::WatchEventList*>(malloc(sizeof(watch::WatchEventList)));
            if (!event_list_) {
                AKLOG_ERRORN("PlayerEvent::emit_hot_reload(): Failed to alloc event ctx(evt_size)");
                return;
            }
            event_list_->size = event_list.size;

            auto events_ = static_cast<watch::WatchEvent*>(
                malloc(sizeof(watch::WatchEvent) * event_list.size));
            if (!events_) {
                AKLOG_ERRORN("PlayerEvent::emit_hot_reload(): Failed to alloc event ctx(events)");
                free(event_list_);
                return;
            }
            for (size_t i = 0; i < event_list.size; i++) {
                events_[i] = event_list.events[i];
            }
            event_list_->events = events_;

            evt.ctx = static_cast<void*>(event_list_);

            m_evt_loop->emit_inner_event(evt);
        }

    }
}
