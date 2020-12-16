#pragma once

#include <libakcore/memory.h>

#include <thread>

namespace akashi {
    namespace core {
        struct FrameContext;
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
            static core::owned_ptr<PerfMonitor> p_perf;

          public:
            explicit MainLoop(){};

            virtual ~MainLoop() {
                if (m_th) {
                    delete m_th;
                }
            }

            void run(MainLoopContext ctx) {
                m_th = new std::thread(&MainLoop::mainloop_thread, ctx);
                m_th->detach();
            };

          private:
            static void mainloop_thread(MainLoopContext ctx);

            static bool sync_render(const MainLoopContext& ctx,
                                    const core::FrameContext& frame_ctx);

            static void update_time(MainLoopContext& ctx, const core::FrameContext& frame_ctx);

          private:
            std::thread* m_th = nullptr;
        };
    }
}
