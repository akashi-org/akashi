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

        void MainLoop::mainloop_thread(MainLoopContext ctx) {
            auto [player, state, event, eval_buf] = ctx;

            AKLOG_INFON("Player thread start");

            state->wait_for_evalbuf_dequeue_ready();

            AKLOG_INFON("Player loop start");

            while (true) {
                state->wait_for_play_ready();
                // state->wait_for_audio_play_ready();

                if (eval_buf->is_hungry()) {
                    event->emit_pull_eval_buffer(25);
                }

                const auto current_frame_ctx = eval_buf->back();
                if (MainLoop::sync_render(ctx, current_frame_ctx)) {
                    ctx.state->set_render_completed(false);
                    eval_buf->set_render_buf(current_frame_ctx);
                    ctx.event->emit_update();
                    p_perf->log_render_start();
                    ctx.state->wait_for_render_completed();
                    p_perf->log_render_end("render_time");
                }

                MainLoop::update_time(ctx, current_frame_ctx);
            }
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

            Rational current_time = Rational(0l);
            bool is_play_over = false;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                is_play_over = state->m_prop.trigger_video_reset;
            }

            if (is_play_over) {
                current_time = Rational(0l);
                {
                    std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                    ctx.state->m_prop.current_time = current_time;
                    ctx.state->m_prop.trigger_video_reset = false;
                }
                auto seek_success = eval_buf->seek(current_time);
                if (!seek_success) {
                    AKLOG_ERRORN("MainLoop::update_time(): seek failed!!!");
                }
                AKLOG_DEBUGN("loop detected");
            } else {
                current_time = frame_ctx.pts;
                {
                    std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                    ctx.state->m_prop.current_time = current_time;
                }
                eval_buf->pop();
            }

            p_perf->log_fps(current_time, ctx.player->current_time());
            ctx.event->emit_time_update(current_time);

            return;
        }

    }
}
