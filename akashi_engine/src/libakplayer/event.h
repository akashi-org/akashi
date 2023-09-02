#pragma once

#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>
#include <libakevent/akevent.h>

#include <functional>

namespace akashi {
    namespace eval {
        class AKEval;
    }
    namespace reload {
        class SeekManager;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace watch {
        struct WatchEventList;
    }
    namespace state {
        class AKState;
    }
    namespace player {

        class EvalBuffer;
        class EventLoop;

        struct PlayerEventContext {
            core::borrowed_ptr<state::AKState> state;
            core::borrowed_ptr<EvalBuffer> eval_buf;
            core::borrowed_ptr<buffer::AVBuffer> buffer;
        };

        class PlayerEvent final : public event::AKEvent {
          public:
            explicit PlayerEvent(const event::EventCallback& evt_cb, void* evt_ctx);
            virtual ~PlayerEvent();

            void run(PlayerEventContext ctx);

            void close_and_wait();

            // [XXX] DO NOT name this 'emit'
            // a compile error will occur when used by Qt
            void emit_event(event::Event& evt) {
                {
                    std::lock_guard<std::mutex> lock(m_evt_prop.mtx);
                    if (m_evt_prop.evt_cb.cb) {
                        m_evt_prop.evt_cb.cb(m_evt_prop.evt_ctx, evt);
                    }
                }
                return;
            };

            void emit_time_update(const core::Rational& time) override {
                event::Event evt;
                evt.name = event::EventName::TIME_UPDATE;
                evt.int64_value = time.num();
                evt.int64_value2 = time.den();
                this->emit_event(evt);
            }

            void emit_set_render_prof(const core::RenderProfile& render_prof) override {
                event::Event evt;
                evt.name = event::EventName::SET_RENDER_PROF;
                evt.render_prof = render_prof;
                this->emit_event(evt);
            }

            void emit_update(void) override {
                event::Event evt;
                evt.name = event::EventName::UPDATE;
                this->emit_event(evt);
            }

            void emit_change_play_state(const state::PlayState& play_state) override {
                event::Event evt;
                evt.name = event::EventName::CHANGE_PLAY_STATE;
                evt.play_state = play_state;
                this->emit_event(evt);
            }

            void emit_seek_completed(void) override {
                event::Event evt;
                evt.name = event::EventName::SEEK_COMPLETED;
                this->emit_event(evt);
            }

            void emit_pull_render_profile(void);

            void emit_pull_eval_buffer(size_t length);

            void emit_seek(const core::Rational& seek_time);

            void emit_hot_reload(const watch::WatchEventList& event_list);

            void emit_inline_eval(const std::string& fpath, const std::string& elem_name);

          private:
            struct {
                event::EventCallback evt_cb = {};
                void* evt_ctx = nullptr;
                std::mutex mtx;
            } m_evt_prop;

            core::owned_ptr<EventLoop> m_evt_loop;
        };
    }
}
