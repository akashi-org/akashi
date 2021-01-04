#include "./hr_manager.h"

#include "../eval_buffer.h"
#include "../event.h"

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
            std::vector<watch::WatchEvent> shader_events;
            for (size_t i = 0; i < event_list.size; i++) {
                std::cmatch m;
                if (std::regex_match(event_list.events[i].file_path, m, std::regex(".*\\.py$"))) {
                    python_events.push_back(event_list.events[i]);
                }
                if (std::regex_match(event_list.events[i].file_path, m,
                                     std::regex(".*\\.(vert|tesc|tese|geom|frag|comp)$"))) {
                    shader_events.push_back(event_list.events[i]);
                }
            }

            if (shader_events.size() > 0) {
                this->reload_shader(shader_events);
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
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                current_time = m_state->m_prop.current_time;
                entry_path = m_state->m_prop.eval_state.config.entry_path;
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

            auto profile = m_eval->render_prof(entry_path.to_abspath().to_str());

            // [XXX] render_prof is updated in emit_set_render_prof,
            // but for the first time call of pull_eval_buffer, update render_prof here also
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.render_prof = profile;
            }

            m_event->emit_set_render_prof(profile); // be careful that decode_ready is called

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

            // m_event->emit_update();
            // m_state->set_play_ready(true);
            // {
            //     std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
            //     m_state->m_prop.play_stopped = false;
            // }
            // m_audio->play();

            return;
        }

        void HRManager::reload_shader(const std::vector<watch::WatchEvent>& events) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.shader_reload = true;
                for (const auto& event : events) {
                    m_state->m_prop.updated_shader_paths.push_back(event.file_path);
                }
            }
            m_event->emit_update();
        }
    }

}
