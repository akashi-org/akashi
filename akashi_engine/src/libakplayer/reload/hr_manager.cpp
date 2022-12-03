#include "./hr_manager.h"
#include "./utils.h"

#include "../eval_buffer.h"
#include "../event.h"
#include "../loop/event_loop.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>
#include <libakbuffer/video_queue.h>
#include <libakaudio/akaudio.h>
#include <libakeval/akeval.h>
#include <libakwatch/item.h>

#include <regex>

using namespace akashi::core;

namespace akashi {
    namespace player {
        HRManager::HRManager(core::borrowed_ptr<state::AKState> state,
                             core::borrowed_ptr<buffer::AVBuffer> buffer,
                             core::borrowed_ptr<PlayerEvent> event,
                             core::borrowed_ptr<EvalBuffer> eval_buf,
                             core::borrowed_ptr<eval::AKEval> eval)
            : m_state(state), m_buffer(buffer), m_event(event), m_eval_buf(eval_buf), m_eval(eval) {
        }

        HRManager::~HRManager() {}

        void HRManager::reload(const watch::WatchEventList& event_list) {
            std::vector<watch::WatchEvent> python_events;
            for (size_t i = 0; i < event_list.size; i++) {
                std::cmatch m;
                // skip reloading for config file
                // if (std::string(event_list.events[i].file_path) ==
                //     std::string(m_state->m_conf_path.to_str())) {
                //     continue;
                // }
                if (std::regex_match(event_list.events[i].file_path, m, std::regex(".*\\.py$"))) {
                    python_events.push_back(event_list.events[i]);
                }
            }

            if (python_events.size() > 0) {
                this->reload_python(python_events);
            }
        }

        void HRManager::reload_python(const std::vector<watch::WatchEvent>& events) {
            reload::ReloadContext rctx{.state = m_state,
                                       .buffer = m_buffer,
                                       .event = m_event,
                                       .eval_buf = m_eval_buf,
                                       .eval = m_eval};

            m_event->emit_change_play_state(state::PlayState::PAUSED);
            m_state->wait_for_not_play_ready();

            // start seek
            {
                m_state->set_seek_completed(false);

                m_state->set_evalbuf_dequeue_ready(false);

                // avbuffer update
                auto dummy_time = Rational(0l);
                reload::reload_avbuffer(rctx, dummy_time, true);

                // kron_reload
                m_eval->reload(events);
                reload::exec_global_eval(rctx);

                Rational current_time;
                Rational duration;
                {
                    std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                    current_time = m_state->m_prop.current_time;
                    duration = m_state->m_prop.render_prof.duration;
                }

                // time update
                if (current_time >= duration) {
                    current_time = Rational(0l);
                    reload::time_update(rctx, current_time);
                }

                reload::exec_local_eval(rctx, current_time, true);

                // restart decode loop
                m_state->set_decode_loop_can_continue(true, true);

                // kick render
                reload::render_update(rctx);

                // end seek
                m_state->set_seek_completed(true);
            }

            return;
        }

        bool HRManager::reload_inline(const InnerEventInlineEvalContext& /*inline_eval_ctx*/) {
            AKLOG_ERRORN("Not implemented!");
            return false;
        }
    }

}
