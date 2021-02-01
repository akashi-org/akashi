#pragma once

#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakcore/audio.h>
#include <libakcore/path.h>
#include <libakcore/hw_accel.h>

#include <mutex>
#include <condition_variable>
#include <atomic>

#define AK_DEF_SYNC_STATE(name, v_type, v_init)                                                    \
  private:                                                                                         \
    struct {                                                                                       \
        v_type value = v_init;                                                                     \
        std::mutex mtx;                                                                            \
        std::condition_variable cv;                                                                \
    } m_state_##name;                                                                              \
                                                                                                   \
  public:                                                                                          \
    void set_##name(v_type v, bool notify_on_false = false) {                                      \
        {                                                                                          \
            std::lock_guard<std::mutex> lock(m_state_##name.mtx);                                  \
            m_state_##name.value = v;                                                              \
        }                                                                                          \
        if (v || notify_on_false) {                                                                \
            m_state_##name.cv.notify_all();                                                        \
        }                                                                                          \
    };                                                                                             \
    v_type get_##name() {                                                                          \
        v_type res = v_init;                                                                       \
        {                                                                                          \
            std::lock_guard<std::mutex> lock(m_state_##name.mtx);                                  \
            res = m_state_##name.value;                                                            \
        }                                                                                          \
        return res;                                                                                \
    };                                                                                             \
    void wait_for_##name() {                                                                       \
        std::unique_lock<std::mutex> lock(m_state_##name.mtx);                                     \
        while (!m_state_##name.value) {                                                            \
            m_state_##name.cv.wait(lock);                                                          \
        }                                                                                          \
    }                                                                                              \
    void wait_for_not_##name() {                                                                   \
        std::unique_lock<std::mutex> lock(m_state_##name.mtx);                                     \
        while (m_state_##name.value) {                                                             \
            m_state_##name.cv.wait(lock);                                                          \
        }                                                                                          \
    }

namespace akashi {

    namespace core {
        struct AKConf;
    }

    namespace state {

        enum class PlayState { NONE = -2, STOPPED = -1, PAUSED, PLAYING };

        struct EvalConfig {
            core::Path include_dir = core::Path("");
            core::Path entry_path = core::Path("");
        };

        struct EvalState {
            EvalConfig config;
        };

        struct PlayerProperty {
            EvalState eval_state;

            using Rational = core::Rational;
            using RenderProfile = core::RenderProfile;

            RenderProfile render_prof;

            int video_width = 1920;

            int video_height = 1080;

            Rational fps = Rational(24, 1);

            std::string default_font_path;

            bool enable_loop = true;

            size_t video_max_queue_size = 1024 * 1024 * 300; // 300mb

            size_t video_max_queue_count = 64;

            size_t audio_max_queue_size = 1024 * 1024 * 100; // 100mb

            // used for dequeueing
            size_t loop_cnt = 0;

            // [TODO] any other ways to handle this?
            // set to true when play over is detected in akaudio
            // set to false when handled properly in player loop
            bool trigger_video_reset = false;

            /**
             * current time to be displayed to the user
             */
            Rational current_time = Rational(0, 1);

            /**
             * elapsed time since the playback started (excluding when it was stopped)
             * used for AV-Sync
             */
            Rational elapsed_time;

            bool done_pull_render_ctx = false;

            // [TODO] should be replaced with uuid?
            uint64_t seek_id = 0; // for checking whether seek is done

            bool seek_success = true; // for checking whether seek succeeded in seek manager

            bool need_first_render = false;

            bool shader_reload = false; // temporary

            std::vector<const char*> updated_shader_paths;
        };

        struct AtomicState {
            std::atomic<int64_t> bytes_played = 0;

            std::atomic<core::Fraction> start_time{{.num = 0, .den = 1}};

            std::atomic<size_t> current_atom_index = 0;

            std::atomic<core::AKAudioSpec> audio_spec;

            std::atomic<PlayState> audio_play_state{PlayState::STOPPED};

            std::atomic<PlayState> icon_play_state{PlayState::STOPPED};

            std::atomic<double> volume = 0.5;

            std::atomic<bool> ui_can_seek = true;

            std::atomic<core::VideoDecodeMethod> decode_method = core::VideoDecodeMethod::NONE;
        };

        class AKState final {
            AK_DEF_SYNC_STATE(evalbuf_dequeue_ready, bool, false)
            AK_DEF_SYNC_STATE(play_ready, bool, false)

            AK_DEF_SYNC_STATE(render_completed, bool, false)
            AK_DEF_SYNC_STATE(eval_completed, bool, true)
            AK_DEF_SYNC_STATE(seek_completed, bool, true)

            // [TODO] atomic?
            AK_DEF_SYNC_STATE(audio_play_ready, bool, false)

            AK_DEF_SYNC_STATE(video_decode_ready, bool, true)
            AK_DEF_SYNC_STATE(audio_decode_ready, bool, true)

          public:
            PlayerProperty m_prop;
            std::mutex m_prop_mtx;

            AtomicState m_atomic_state;

          public:
            explicit AKState(const core::AKConf& akconf);
            virtual ~AKState();
        };

    }
}
