#pragma once

#include <libakcore/memory.h>

#include <vector>

typedef struct pa_stream pa_stream;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;
typedef struct pa_context pa_context;

namespace akashi {
    namespace core {
        class Rational;
        struct AtomProfile;
    }
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
        struct AudioQueueData;
    }
    namespace event {
        class AKEvent;
    }
    namespace state {
        class AKState;
        enum class PlayState;
    }
    namespace audio {

        class PulseAudioContext;
        class CallbackContext {
          public:
            // [TODO] event is used only for pa's player_pause. can it be deleted?
            explicit CallbackContext(core::borrowed_ptr<PulseAudioContext> audio_ctx,
                                     core::borrowed_ptr<state::AKState> state,
                                     core::borrowed_ptr<buffer::AVBuffer> buffer,
                                     core::borrowed_ptr<event::AKEvent> event)
                : m_audio_ctx(audio_ctx), m_state(state), m_buffer(buffer), m_event(event){};

            // [XXX] all the resources in this class should be managed by PulseAudioContext
            virtual ~CallbackContext() = default;

            void destroy(void);

            core::Rational current_time(void) const;

            const char* get_pa_error(void) const;

            size_t bytes_per_second(void) const;

            core::Rational to_pts(size_t bytes) const;

            bool is_play_over(void) const;

            void incr_loop_cnt(void);

            size_t loop_cnt(void) const;

            void incr_bytes_played(int64_t bytes);

            const std::vector<core::AtomProfile>& atom_profiles(void) const;

            core::AtomProfile select_current_atom(const size_t requested_bytes);

            state::PlayState state(void) const;

            double volume(void) const;

            bool is_buf_empty(std::string layer_uuid);

            const buffer::AVBufferData& front_buf(std::string layer_uuid, bool check_empty);

            bool seek_buf(std::string layer_uuid, const core::Rational& seek_pts);

            buffer::AudioQueueData& queue(std::string layer_uuid);

            void pop_front_buf(std::string layer_uuid);

            void player_pause(void);

          private:
            core::borrowed_ptr<PulseAudioContext> m_audio_ctx;
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;
            core::borrowed_ptr<event::AKEvent> m_event;
        };

        void stream_write_cb(pa_stream* stream, size_t requested_bytes, void* userdata);

    }
}
