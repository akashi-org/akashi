#pragma once

#include <libakcore/memory.h>

#include <thread>
#include <functional>
#include <mutex>

namespace akashi {
    namespace state {
        class AKState;
    }

    namespace player {

        class PlayerEvent;
        struct WatchLoopContext {
            core::borrowed_ptr<PlayerEvent> event;
            core::borrowed_ptr<state::AKState> state;
        };

        class WatchLoop final {
          public:
            explicit WatchLoop() = default;

            virtual ~WatchLoop() = default;

            void close_and_wait() {
                if (m_th) {
                    {
                        std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                        if (m_on_thread_exit.func) {
                            m_on_thread_exit.func(m_on_thread_exit.ctx);
                        }
                    }
                    m_th->join();
                    delete m_th;
                    m_th = nullptr;
                }
            }

            void run(WatchLoopContext ctx) {
                m_th = new std::thread(&WatchLoop::watch_thread, ctx, this);
            };

            void set_on_thread_exit(std::function<void(void*)> on_thread_exit, void* ctx) {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    m_on_thread_exit.func = on_thread_exit;
                    m_on_thread_exit.ctx = ctx;
                }
                return;
            }

          private:
            static void watch_thread(WatchLoopContext ctx, WatchLoop* loop);

          private:
            std::thread* m_th = nullptr;
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;
        };
    }
}
