#include "./akstate.h"

#include <libakcore/config.h>
#include <libakcore/path.h>
#include <libakcore/rational.h>

namespace akashi {
    namespace state {

        AKState::AKState(const core::AKConf& akconf) {
            m_prop.eval_state.config.entry_path = core::Path(akconf.general.entry_file);
            m_prop.eval_state.config.include_dir = core::Path(akconf.general.include_dir);

            m_prop.fps = to_rational(akconf.video.fps);
            m_prop.video_width = akconf.video.resolution.first;
            m_prop.video_height = akconf.video.resolution.second;
            m_prop.default_font_path = akconf.video.default_font_path;

            m_atomic_state.audio_spec = {akconf.audio.format, akconf.audio.sample_rate,
                                         akconf.audio.channels, akconf.audio.channel_layout};

            m_prop.enable_loop = akconf.playback.enable_loop;
            m_atomic_state.volume = akconf.playback.gain;
            m_atomic_state.decode_method = akconf.playback.decode_method;
            m_prop.video_max_queue_size = akconf.playback.video_max_queue_size;
            m_prop.video_max_queue_count = akconf.playback.video_max_queue_count;
            m_prop.audio_max_queue_size = akconf.playback.audio_max_queue_size;

            m_encode_conf = akconf.encode;
        }

        AKState::~AKState() {
            // m_state_evalbuf_dequeue_ready.cv.notify_all();
            // m_state_play_ready.cv.notify_all();
            // m_state_render_completed.cv.notify_all();
            // m_state_eval_completed.cv.notify_all();
            // m_state_seek_completed.cv.notify_all();
            // m_state_audio_play_ready.cv.notify_all();
            // m_state_video_decode_ready.cv.notify_all();
            // m_state_audio_decode_ready.cv.notify_all();
            // m_state_producer_finished.cv.notify_all();
            // m_state_consumer_finished.cv.notify_all();
        }

    }
}
