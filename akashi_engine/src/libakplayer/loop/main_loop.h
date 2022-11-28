#pragma once

#include <libakcore/memory.h>
#include <libakstate/akstate.h>

#include <thread>
#include <atomic>

namespace akashi {
    namespace core {
        struct FrameContext;
        class PerfMonitor;
    }
    namespace state {
        class AKState;
    }
    namespace player {

        class AKPlayer;
        class PlayerEvent;
        class EvalBuffer;
        class PerfMonitor;

        struct MainLoopContext {
            core::borrowed_ptr<AKPlayer> player;
            core::borrowed_ptr<state::AKState> state;
            core::borrowed_ptr<PlayerEvent> event;
            core::borrowed_ptr<EvalBuffer> eval_buf;
        };

        class MainLoop final {
          private:
            static core::owned_ptr<core::PerfMonitor> p_perf;

          public:
            explicit MainLoop(core::borrowed_ptr<state::AKState> state) : m_state(state){};

            virtual ~MainLoop() {
                if (m_th) {
                    delete m_th;
                }
            }

            void close_and_wait() {
                if (m_th) {
                    m_is_alive.store(false);
                    m_state->set_play_ready(true, true);
                    m_state->set_audio_play_ready(true, true);
                    m_state->set_render_completed(true, true);
                    m_state->set_evalbuf_dequeue_ready(true, true);

                    m_th->join();
                    delete m_th;
                    m_th = nullptr;
                }
            }

            void run(MainLoopContext ctx) {
                m_th = new std::thread(&MainLoop::mainloop_thread, ctx, this);
            };

          private:
            static void mainloop_thread(MainLoopContext ctx, MainLoop* loop);

            static bool sync_render(const MainLoopContext& ctx,
                                    const core::FrameContext& frame_ctx);

            static void update_time(MainLoopContext& ctx, const core::FrameContext& frame_ctx);

          private:
            std::thread* m_th = nullptr;
            std::atomic<bool> m_is_alive = true;
            core::borrowed_ptr<state::AKState> m_state;
        };
    }
}
