#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <vector>

typedef struct pa_stream pa_stream;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;
typedef struct pa_context pa_context;

namespace akashi {
    namespace core {
        struct AtomProfile;
        struct LayerProfile;
    }
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
        class AudioQueue;
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

            core::Rational current_time(void) const;

            const char* get_pa_error(void) const;

            size_t bytes_per_second(void) const;

            void set_loop_cnt(size_t cnt);

            size_t loop_cnt(void) const;

            void incr_bytes_played(int64_t bytes);

            void set_bytes_played(int64_t bytes);

            bool audio_play_over(void);

            void set_audio_play_over(bool play_over);

            bool video_play_over(void);

            const std::vector<core::AtomProfile>& atom_profiles(void) const;

            state::PlayState state(void) const;

            double volume(void) const;

            core::Rational start_time(void);

            void set_start_time(const core::Rational& start_time);

            core::borrowed_ptr<buffer::AudioQueue> aq(void);

            void check_audio_play_ready(void);

            void player_pause(void);

          private:
            core::borrowed_ptr<PulseAudioContext> m_audio_ctx;
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;
            core::borrowed_ptr<event::AKEvent> m_event;
        };

        struct WBSegment {
            uint8_t* buf = nullptr;
            size_t buf_size = 0;
            core::Rational from_pts = core::Rational(0l);
            core::Rational to_pts = core::Rational(0l);
        };

        struct WBSegmentSlice {
            uint8_t* buf = nullptr;
            size_t buf_size = 0;
        };

        struct AudioProfile {
            core::Rational current_time = core::Rational(0l);
            size_t bytes_per_second = 0;
            core::Rational duration = core::Rational(0l);
        };

        size_t dur_to_bytes(const size_t& bytes_per_second, const core::Rational& duration);

        core::Rational bytes_to_dur(const size_t& bytes_per_second, const size_t& bytes);

        bool find_segment(WBSegment* segment, bool* is_play_over, const AudioProfile& prof,
                          uint8_t* mask_buf, size_t requested_bytes);

        void segment_fill(const WBSegmentSlice& slice, core::borrowed_ptr<buffer::AudioQueue> aq,
                          const core::LayerProfile& layer);

        void stream_write_cb(pa_stream* stream, size_t requested_bytes, void* userdata);

    }
}
