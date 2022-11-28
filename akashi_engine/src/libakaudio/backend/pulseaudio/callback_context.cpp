#include "./callback.h"
#include "./context.h"
#include "./util.h"

#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>

#include <libakstate/akstate.h>
#include <libakplayer/event.h>
#include <libakcore/logger.h>
#include <libakcore/rational.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        core::Rational CallbackContext::current_time(void) const {
            return m_audio_ctx->current_time();
        };

        const char* CallbackContext::get_pa_error(void) const {
            return m_audio_ctx->get_pa_error();
        };

        size_t CallbackContext::bytes_per_second(void) const {
            return m_audio_ctx->bytes_per_second();
        };

        void CallbackContext::incr_bytes_played(int64_t bytes) {
            m_state->m_atomic_state.bytes_played.fetch_add(bytes);
        }

        void CallbackContext::set_bytes_played(int64_t bytes) {
            m_state->m_atomic_state.bytes_played.store(bytes);
        }

        bool CallbackContext::audio_play_over(void) {
            return m_state->m_atomic_state.audio_play_over.load();
        }

        void CallbackContext::set_audio_play_over(bool play_over) {
            m_state->m_atomic_state.audio_play_over.store(play_over);
        }

        bool CallbackContext::video_play_over(void) {
            // [TODO] lock free?
            return !m_state->get_play_ready();

            // return m_state->m_atomic_state.video_play_over.load();
        }

        const std::vector<core::AtomProfile>& CallbackContext::atom_profiles(void) const {
            // [TODO] lock?
            return m_state->m_prop.render_prof.atom_profiles;
        };

        core::borrowed_ptr<buffer::AudioQueue> CallbackContext::aq(void) {
            return core::borrowed_ptr(m_buffer->aq);
        }

        core::Rational CallbackContext::start_time(void) {
            return m_state->m_atomic_state.start_time.load();
        }

        void CallbackContext::set_start_time(const core::Rational& start_time) {
            m_state->m_atomic_state.start_time.store(start_time);
        }

        state::PlayState CallbackContext::state(void) const {
            return m_state->m_atomic_state.audio_play_state.load();
        }

        double CallbackContext::volume(void) const { return m_state->m_atomic_state.volume.load(); }

        void CallbackContext::check_audio_play_ready(void) {
            auto queue_size = m_buffer->aq->total_queue_size();
            bool can_play = queue_size >= MIN_PLAYABLE_QUEUE_SIZE;

            m_state->set_audio_play_ready(can_play);
            if (!can_play) {
                this->player_pause();
            }
        };

        void CallbackContext::player_pause(void) {
            if (m_event) {
                m_event->emit_change_play_state(state::PlayState::PAUSED);
            } else {
                AKLOG_WARNN("CallbackContext::player_pause(): m_event is null");
                return;
            }
        }

    }

}
