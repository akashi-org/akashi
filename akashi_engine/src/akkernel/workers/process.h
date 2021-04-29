#pragma once

#include <libakcore/memory.h>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>

namespace akashi {
    namespace kernel {

        class KernelEventQueue;
        class KernelState;

        struct ProcessWorkerContext {
            core::borrowed_ptr<KernelState> state;
            core::borrowed_ptr<KernelEventQueue> queue;
        };

        class ProcessWorker final {
          private:
            struct ExitContext;
            struct PrivProcess;

          public:
            explicit ProcessWorker(){};

            virtual ~ProcessWorker() { this->terminate(); }

            void terminate() {
                if (m_thread_alive) {
                    {
                        std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                        if (m_on_thread_exit.func) {
                            m_on_thread_exit.func(m_on_thread_exit.ctx);
                        }
                        m_on_thread_exit.func = nullptr;
                        m_on_thread_exit.ctx = nullptr;
                        m_thread_alive = false;
                    }
                }
            };

            void run(ProcessWorkerContext ctx) noexcept(false) {
                if (m_thread_alive) {
                    throw std::runtime_error("Process already exists");
                }
                std::thread(&ProcessWorker::process_thread, ctx, this).detach();
                m_thread_alive = true;
            };

            void set_on_thread_exit(std::function<void(void*)> on_thread_exit, void* ctx) {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    m_on_thread_exit.func = on_thread_exit;
                    m_on_thread_exit.ctx = ctx;
                }
                return;
            }

            bool alive(void) const { return m_thread_alive; }

            std::string pass_through(const std::string& req_str);

          private:
            static void process_thread(ProcessWorkerContext ctx, ProcessWorker* worker);

            static void exit_thread(ExitContext& exit_ctx);

          private:
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;
            struct {
                PrivProcess* value = nullptr;
                std::mutex mtx;
            } m_priv_process;
            std::atomic<bool> m_thread_alive = false;
        };

    }
}
