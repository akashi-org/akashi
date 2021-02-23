#pragma once

#include <libakcore/memory.h>
#include <thread>
#include <functional>
#include <mutex>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace codec {
        class AKEncoder;
    }
    namespace encoder {

        class EncodeQueue;

        struct ConsumeLoopContext {
            core::borrowed_ptr<state::AKState> state;
            core::borrowed_ptr<codec::AKEncoder> encoder;
            core::borrowed_ptr<EncodeQueue> queue;
        };

        struct ExitContext;
        class ConsumeLoop final {
          public:
            explicit ConsumeLoop(){};

            virtual ~ConsumeLoop() { this->terminate(); }

            void terminate() {
                if (!m_thread_exited) {
                    {
                        std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                        if (m_on_thread_exit.func) {
                            m_on_thread_exit.func(m_on_thread_exit.ctx);
                        }
                        m_on_thread_exit.func = nullptr;
                        m_thread_exited = true;
                    }
                }
                if (m_th) {
                    delete m_th;
                    m_th = nullptr;
                }
            };

            void run(ConsumeLoopContext ctx) {
                m_th = new std::thread(&ConsumeLoop::consume_thread, ctx, this);
                m_th->detach();
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
            static void consume_thread(ConsumeLoopContext ctx, ConsumeLoop* loop);

            static void exit_thread(ExitContext& exit_ctx);

          private:
            std::thread* m_th = nullptr;
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;
            bool m_thread_exited = false;
        };

    }
}
