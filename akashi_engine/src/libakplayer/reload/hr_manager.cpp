#include "./hr_manager.h"

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

        static auto eval_krons(const core::Rational& seek_time, size_t length,
                               core::borrowed_ptr<state::AKState> state,
                               core::borrowed_ptr<eval::AKEval> eval) {
            Rational fps;
            Rational duration;
            core::Path entry_path{""};
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                fps = state->m_prop.fps;
                duration = to_rational(state->m_prop.render_prof.duration);
                entry_path = state->m_prop.eval_state.config.entry_path;
            }

            Rational start_time = seek_time;
            auto is_init_pts = start_time.num() == 0 ? true : false;
            if (!is_init_pts) {
                start_time += (Rational(1, 1) / fps);
            }

            return eval->eval_krons(entry_path.to_abspath().to_str(), start_time, fps.to_decimal(),
                                    duration, length);
        }

        static void update_current_atom_index(const core::Rational& current_time,
                                              core::borrowed_ptr<state::AKState> state) {
            auto current_atom_index = state->m_atomic_state.current_atom_index.load();
            std::vector<core::AtomProfile> atom_profiles;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                atom_profiles = state->m_prop.render_prof.atom_profiles;
            }

            while (true) {
                const auto atom_profile = atom_profiles[current_atom_index];
                if (to_rational(atom_profile.from) <= current_time &&
                    current_time <= to_rational(atom_profile.to)) {
                    break;
                } else {
                    if (current_atom_index >= atom_profiles.size() - 1) {
                        current_atom_index = 0;
                    } else {
                        current_atom_index += 1;
                    }
                }
            }

            state->m_atomic_state.current_atom_index.store(current_atom_index);
        }

        void HRManager::reload(const watch::WatchEventList& event_list) {
            std::vector<watch::WatchEvent> python_events;
            for (size_t i = 0; i < event_list.size; i++) {
                std::cmatch m;
                // skip reloading for config file
                if (std::string(event_list.events[i].file_path) ==
                    std::string(m_state->m_conf_path.to_str())) {
                    continue;
                }
                if (std::regex_match(event_list.events[i].file_path, m, std::regex(".*\\.py$"))) {
                    python_events.push_back(event_list.events[i]);
                }
            }

            if (python_events.size() > 0) {
                this->reload_python(python_events);
            }
        }

        void HRManager::reload_python(const std::vector<watch::WatchEvent>& events) {
            m_event->emit_change_play_state(state::PlayState::PAUSED);
            m_state->wait_for_not_play_ready();

            Rational current_time;
            core::Path entry_path{""};
            std::string elem_name{""};
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                current_time = m_state->m_prop.current_time;
                entry_path = m_state->m_prop.eval_state.config.entry_path;
                elem_name = m_state->m_prop.eval_state.config.elem_name;
            }

            // start seek
            m_state->set_seek_completed(false);
            m_state->set_evalbuf_dequeue_ready(false);
            // m_state->set_play_ready(false);
            // m_audio->pause();

            update_current_atom_index(current_time, m_state);

            // avbuffer update
            m_buffer->vq->clear(true);
            m_buffer->aq->clear(true);

            // kron_reload
            m_eval->reload(events);

            auto profile = m_eval->render_prof(entry_path.to_abspath().to_str(), elem_name);

            // [XXX] render_prof is updated in emit_set_render_prof,
            // but for the first time call of pull_eval_buffer, update render_prof here also
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.render_prof = profile;
            }

            m_event->emit_set_render_prof(profile); // be careful that decode_ready is called

            m_state->set_decode_layers_not_empty(core::has_layers(profile), true);

            m_eval_buf->clear();
            // m_event->emit_pull_eval_buffer(50);
            // m_state->wait_for_evalbuf_dequeue_ready();
            auto ebufs = eval_krons(current_time, 50, m_state, m_eval);
            m_eval_buf->push(ebufs);
            m_eval_buf->pop();
            m_eval_buf->set_render_buf(ebufs[0]);
            // m_state->set_evalbuf_dequeue_ready(true);
            //

            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.need_first_render = true;
            }

            // end seek
            m_state->set_evalbuf_dequeue_ready(true);
            m_state->set_seek_completed(true);

            m_event->emit_update();
            // m_state->set_play_ready(true);
            // {
            //     std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
            //     m_state->m_prop.play_stopped = false;
            // }
            // m_audio->play();

            return;
        }

        bool HRManager::reload_inline(const InnerEventInlineEvalContext& inline_eval_ctx) {
            auto profile =
                m_eval->render_prof(inline_eval_ctx.file_path, inline_eval_ctx.elem_name);
            if (profile.atom_profiles.size() == 0) {
                return false;
            }

            m_event->emit_change_play_state(state::PlayState::PAUSED);
            m_state->wait_for_not_play_ready();

            Rational seek_time = core::Rational(0, 1);

            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.current_time = seek_time;
                m_state->m_prop.elapsed_time = seek_time;
                m_state->m_prop.eval_state.config.entry_path = Path(inline_eval_ctx.file_path);
                m_state->m_prop.eval_state.config.elem_name = inline_eval_ctx.elem_name;
                // [XXX] reset loop_cnt here
                m_state->m_atomic_state.decode_loop_cnt = 0;
                m_state->m_atomic_state.play_loop_cnt = 0;
            }

            // start seek
            m_state->set_seek_completed(false);
            m_state->set_evalbuf_dequeue_ready(false);
            // m_state->set_play_ready(false);
            // m_audio->pause();

            update_current_atom_index(seek_time, m_state);
            m_event->emit_time_update(seek_time);

            m_state->m_atomic_state.start_time.store(
                {.num = seek_time.num(), .den = seek_time.den()});
            m_state->m_atomic_state.bytes_played.store(0);

            // avbuffer update
            m_buffer->vq->clear(true);
            m_buffer->aq->clear(true);

            // [XXX] render_prof is updated in emit_set_render_prof,
            // but for the first time call of pull_eval_buffer, update render_prof here also
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.render_prof = profile;
            }

            m_event->emit_set_render_prof(profile); // be careful that decode_ready is called

            m_state->set_decode_layers_not_empty(core::has_layers(profile), true);

            m_eval_buf->clear();
            // m_event->emit_pull_eval_buffer(50);
            // m_state->wait_for_evalbuf_dequeue_ready();
            auto ebufs = eval_krons(seek_time, 50, m_state, m_eval);
            m_eval_buf->push(ebufs);
            m_eval_buf->pop();
            m_eval_buf->set_render_buf(ebufs[0]);
            // m_state->set_evalbuf_dequeue_ready(true);
            //

            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.need_first_render = true;
            }

            // end seek
            m_state->set_evalbuf_dequeue_ready(true);
            m_state->set_seek_completed(true);

            m_event->emit_update();

            return true;
        }
    }

}
