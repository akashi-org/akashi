#include "./main_loop.h"

#include "../akplayer.h"
#include "../event.h"
#include "../eval_buffer.h"

#include <libakcore/logger.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/perf.h>
#include <libakeval/item.h>
#include <libakstate/akstate.h>

using namespace akashi::core;

namespace akashi {
    namespace player {

        core::owned_ptr<core::PerfMonitor> MainLoop::p_perf(new core::PerfMonitor);

        void MainLoop::mainloop_thread(MainLoopContext ctx, MainLoop* loop) {
            auto [player, state, event, eval_buf] = ctx;

            AKLOG_INFON("Player thread start");

            state->wait_for_kron_ready();

            AKLOG_INFON("Player loop start");

            while (true) {
                state->wait_for_play_ready();
                // state->wait_for_audio_play_ready();
                if (!loop->m_is_alive) {
                    break;
                }

                eval_buf->fetch_render_buf();
                const auto& current_frame_ctx = eval_buf->render_buf();
                if (MainLoop::sync_render(ctx, current_frame_ctx)) {
                    ctx.state->set_render_completed(false);
                    ctx.event->emit_update();
                    p_perf->log_render_start();
                    ctx.state->wait_for_render_completed();
                    p_perf->log_render_end("render_time");
                }

                MainLoop::update_time(ctx, current_frame_ctx);
            }

            AKLOG_INFON("Player loop successfully exited");
        }

        bool MainLoop::sync_render(const MainLoopContext& ctx,
                                   const core::FrameContext& frame_ctx) {
            auto audio_time = ctx.player->current_time();
            auto delay = frame_ctx.pts - audio_time;

            // auto elapsed = audio_time - p_perf->elapsed_time();

            // skip
            if (delay < Rational(-100, 1000)) {
                p_perf->log_delay(delay, 1);
                AKLOG_INFON("sync_render(): 🐥🐥🐥frame dropped");
                return false;
            }
            // wait
            else if (delay >= Rational(0l)) {
                // fps keeper
                Rational adjusted_delay = delay;
                // {
                //     std::lock_guard<std::mutex> lk(player_ctx->m_prop_mtx);
                //     adjusted_delay =
                //         std::min(Rational(1l) / player_ctx->m_prop.render_prof.fps, delay);
                // }

                auto factor = Rational(1000l);
                auto sleep_ms = (long)((adjusted_delay)*factor).to_decimal();
                // [TODO] if timescale is not 10*3, the calculation below must be changed
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

                p_perf->log_delay(adjusted_delay, 0);
                return true;
            }

            AKLOG_INFON("sync_render(): 🐸🐸🐸no wait");
            p_perf->log_delay(delay, 0);
            return true;
        }

        void MainLoop::update_time(MainLoopContext& ctx, const core::FrameContext& frame_ctx) {
            auto [player, state, event, eval_buf] = ctx;

            Rational current_time = frame_ctx.pts;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                size_t cur_frame_num = (frame_ctx.pts * state->m_prop.fps).to_decimal();
                auto max_frame_idx = state->m_prop.max_frame_idx;
                auto fps = state->m_prop.fps;
                bool is_play_over = cur_frame_num >= max_frame_idx;
                // AKLOG_WARN("cur_frame: {}, total_frames: {}, is_play_over: {} ", cur_frame_num,
                //            total_frames, is_play_over);
                state->m_atomic_state.video_play_over = is_play_over;
                if (is_play_over) {
                    ctx.state->set_play_ready(false, true);
                    // ctx.state->m_prop.current_time = Rational(0, 0);
                } else {
                    ctx.state->m_prop.current_time = current_time + (Rational(1, 1) / fps);
                }
            }

            p_perf->log_fps(current_time, ctx.player->current_time());
            ctx.event->emit_time_update(current_time);

            return;
        }

    }
}
