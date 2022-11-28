#include "./seek_manager.h"

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

using namespace akashi::core;

namespace akashi {
    namespace player {
        SeekManager::SeekManager(core::borrowed_ptr<state::AKState> state,
                                 core::borrowed_ptr<buffer::AVBuffer> buffer,
                                 core::borrowed_ptr<PlayerEvent> event,
                                 core::borrowed_ptr<EvalBuffer> eval_buf,
                                 core::borrowed_ptr<eval::AKEval> eval)
            : m_state(state), m_buffer(buffer), m_event(event), m_eval_buf(eval_buf), m_eval(eval) {
        }

        SeekManager::~SeekManager() {}

        static auto eval_krons(const core::Rational& seek_time, size_t length,
                               core::borrowed_ptr<state::AKState> state,
                               core::borrowed_ptr<eval::AKEval> eval) {
            Rational fps;
            Rational duration;
            core::Path entry_path{""};
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                fps = state->m_prop.fps;
                duration = state->m_prop.render_prof.duration;
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

        static void update_current_atom_index(const core::Rational& seek_time, bool is_forward_seek,
                                              core::borrowed_ptr<state::AKState> state) {
            auto current_atom_index = state->m_atomic_state.current_atom_index.load();
            std::vector<core::AtomProfile> atom_profiles;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                atom_profiles = state->m_prop.render_prof.atom_profiles;
            }

            while (true) {
                const auto atom_profile = atom_profiles[current_atom_index];
                if (atom_profile.from <= seek_time && seek_time <= atom_profile.to) {
                    break;
                } else {
                    if (is_forward_seek) {
                        if (current_atom_index >= atom_profiles.size() - 1) {
                            // [TODO] usually, unreachable case
                            current_atom_index = 0;
                        } else {
                            current_atom_index += 1;
                        }
                    } else {
                        if (current_atom_index == 0) {
                            // [TODO] usually, unreachable case
                            current_atom_index = atom_profiles.size() - 1;
                        } else {
                            current_atom_index -= 1;
                        }
                    }
                }
            }

            state->m_atomic_state.current_atom_index.store(current_atom_index);
        }

        void SeekManager::seek(const core::Rational& seek_time) {
            // [XXX] seek_time is assumed to be divisible by fps
            AKLOG_INFO("Play seek: {}({}/{})", seek_time.to_decimal(), seek_time.num(),
                       seek_time.den());

            if (!(this->can_seek(seek_time))) {
                AKLOG_ERRORN("SeekManager::seek() failed: not ready for seeking");
                m_event->emit_seek_completed(); // notify to ui for making the slider movable
                return;
            }

            // start seek
            m_state->set_seek_completed(false);

            // timeupdate
            bool is_forward_seek = true;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                is_forward_seek = seek_time >= m_state->m_prop.current_time;
                m_state->m_prop.current_time = seek_time;
                // [TODO] delta?
                m_state->m_prop.elapsed_time = seek_time;
                m_state->m_prop.seek_id =
                    m_state->m_prop.seek_id == UINT64_MAX ? 0 : m_state->m_prop.seek_id + 1;
            }
            // update_current_atom_index(seek_time, is_forward_seek, m_state);
            m_event->emit_time_update(seek_time);
            // m_audio->seek(seek_time);

            m_state->m_atomic_state.audio_play_over = false;
            m_state->m_atomic_state.start_time.store(Rational{seek_time.num(), seek_time.den()});
            m_state->m_atomic_state.bytes_played.store(0);

            // avbuffer update
            bool avbuffer_seek_success = false;
            if (m_buffer->vq->seek(seek_time) && m_buffer->aq->seek(seek_time)) {
                avbuffer_seek_success = true;
                {
                    std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                    m_state->m_prop.seek_success = true;
                }
            } else {
                m_buffer->vq->clear(true);
                m_buffer->aq->clear(true);
                {
                    std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                    m_state->m_prop.seek_success = false;
                }
            }

            // evalbuffer update
            bool seek_point_found = m_eval_buf->seek(seek_time);

            if (!seek_point_found) {
                m_eval_buf->clear();
                // m_event->emit_pull_eval_buffer(50);
                // m_state->wait_for_evalbuf_dequeue_ready();
                auto ebufs = eval_krons(seek_time, 50, m_state, m_eval);
                m_eval_buf->push(ebufs);
                // m_eval_buf->pop();
                m_eval_buf->set_render_buf(ebufs[0]);
                // m_state->set_evalbuf_dequeue_ready(true);
            }
            m_state->set_evalbuf_dequeue_ready(true);

            // render
            if (avbuffer_seek_success && seek_point_found) {
                m_event->emit_update();
                m_event->emit_seek_completed(); // notify to ui
            } else {
                {
                    std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                    m_state->m_prop.need_first_render = true;
                }
                m_event->emit_update();
            }

            // end seek
            m_state->set_seek_completed(true);

            return;
        }

        bool SeekManager::can_seek(const core::Rational& seek_time) {
            Rational duration;
            bool seek_completed = false;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                duration = m_state->m_prop.render_prof.duration;
                seek_completed = m_state->get_seek_completed(); // [TODO] do we need lock here?
            }

            if (!seek_completed) {
                AKLOG_WARNN("already seeking");
                return false;
            }
            if (seek_time < Rational(0, 1) || seek_time >= duration) {
                AKLOG_ERRORN("seek time is out of range");
                return false;
            }

            return true;
        }

    }
}
