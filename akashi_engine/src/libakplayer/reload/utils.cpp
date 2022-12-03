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

using namespace akashi::core;

namespace akashi::player::reload {

    void time_update(ReloadContext& rctx, const core::Rational& seek_time) {
        {
            std::lock_guard<std::mutex> lock(rctx.state->m_prop_mtx);
            rctx.state->m_prop.current_time = seek_time;
            // [TODO] delta?
            rctx.state->m_prop.elapsed_time = seek_time;
            rctx.state->m_prop.seek_id =
                rctx.state->m_prop.seek_id == UINT64_MAX ? 0 : rctx.state->m_prop.seek_id + 1;
        }
        rctx.event->emit_time_update(seek_time);

        rctx.state->m_atomic_state.audio_play_over = false;
        rctx.state->m_atomic_state.start_time.store(Rational{seek_time.num(), seek_time.den()});
        rctx.state->m_atomic_state.bytes_played.store(0);
    }

    bool reload_avbuffer(ReloadContext& rctx, const core::Rational& seek_time, bool skip_seek) {
        bool avbuffer_seek_success = false;

        if (!skip_seek) {
            if (rctx.buffer->vq->seek(seek_time) && rctx.buffer->aq->seek(seek_time)) {
                avbuffer_seek_success = true;
                {
                    std::lock_guard<std::mutex> lock(rctx.state->m_prop_mtx);
                    rctx.state->m_prop.seek_success = true;
                }
            }
        }

        if (!avbuffer_seek_success) {
            rctx.buffer->vq->clear(true);
            rctx.buffer->aq->clear(true);
            {
                std::lock_guard<std::mutex> lock(rctx.state->m_prop_mtx);
                rctx.state->m_prop.seek_success = false;
            }
        }

        return avbuffer_seek_success;
    }

    void exec_global_eval(ReloadContext& rctx) {
        core::Path entry_path{""};
        std::string elem_name{""};
        Rational fps;
        {
            std::lock_guard<std::mutex> lock(rctx.state->m_prop_mtx);
            entry_path = rctx.state->m_prop.eval_state.config.entry_path;
            elem_name = rctx.state->m_prop.eval_state.config.elem_name;
            fps = rctx.state->m_prop.fps;
        }

        auto profile = rctx.eval->render_prof(entry_path.to_abspath().to_str(), elem_name);

        // [XXX] render_prof is updated in emit_set_render_prof,
        // but for the first time call of pull_eval_buffer, update render_prof here also
        {
            std::lock_guard<std::mutex> lock(rctx.state->m_prop_mtx);
            rctx.state->m_prop.render_prof = profile;
            rctx.state->m_prop.max_frame_idx =
                ((profile.duration * fps) - Rational(1l)).to_decimal();
        }

        rctx.event->emit_set_render_prof(profile); // be careful that decode_ready is called

        rctx.state->set_decode_layers_not_empty(core::has_layers(profile), true);
    }

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

    bool exec_local_eval(ReloadContext& rctx, const core::Rational& seek_time, bool skip_seek) {
        bool seek_point_found = false;
        if (!skip_seek) {
            seek_point_found = rctx.eval_buf->seek(seek_time);
        }

        if (!seek_point_found) {
            rctx.eval_buf->clear();
            auto ebufs = eval_krons(seek_time, 50, rctx.state, rctx.eval);
            rctx.eval_buf->push(ebufs);
            rctx.eval_buf->set_render_buf(ebufs[0]);
        }

        rctx.state->set_evalbuf_dequeue_ready(true);

        return seek_point_found;
    }

    void render_update(ReloadContext& rctx) {
        {
            std::lock_guard<std::mutex> lock(rctx.state->m_prop_mtx);
            rctx.state->m_prop.need_first_render = true;
        }
        rctx.event->emit_update();
    }
}
