#include "./seek_manager.h"
#include "./utils.h"

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

        void SeekManager::seek(const core::Rational& seek_time) {
            // [XXX] seek_time is assumed to be divisible by fps
            AKLOG_INFO("Play seek: {}({}/{})", seek_time.to_decimal(), seek_time.num(),
                       seek_time.den());

            if (!(this->can_seek(seek_time))) {
                AKLOG_ERRORN("SeekManager::seek() failed: not ready for seeking");
                m_event->emit_seek_completed(); // notify to ui for making the slider movable
                return;
            }

            reload::ReloadContext rctx{.state = m_state,
                                       .buffer = m_buffer,
                                       .event = m_event,
                                       .eval_buf = m_eval_buf,
                                       .eval = m_eval};

            {
                // start seek
                m_state->set_seek_completed(false);

                // timeupdate
                reload::time_update(rctx, seek_time);

                // avbuffer update
                reload::reload_avbuffer(rctx, seek_time, false);

                // restart decode loop
                m_state->set_decode_loop_can_continue(true, true);

                // render
                reload::render_update(rctx);
                m_event->emit_seek_completed(); // notify to ui

                // end seek
                m_state->set_seek_completed(true);
            }

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
