#pragma once

#include <libakcore/memory.h>
#include <thread>
#include <functional>
#include <mutex>

namespace akashi {
    namespace kernel {

        constexpr static const uint32_t DEFAULT_ASP_PORT = 1234;

        struct KernelLoopContext {
            std::string config_jstr;
            std::string renderer_path;
            uint32_t port = DEFAULT_ASP_PORT;
            std::string config_path;
        };

        class KernelLoop final {
          private:
            struct ExitContext;

          public:
            explicit KernelLoop(){};

            virtual ~KernelLoop() { this->terminate(); }

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
            };

            void run(KernelLoopContext ctx) {
                std::thread(&KernelLoop::kernel_thread, ctx, this).detach();
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
            static void kernel_thread(KernelLoopContext ctx, KernelLoop* loop);

            static void exit_thread(ExitContext& exit_ctx);

          private:
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;
            bool m_thread_exited = false;
        };

    }
}
