#pragma once

#include <libakcore/memory.h>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

#define AK_DEF_KERNEL_PROCESS_WORKER_STATE(name, v_type, v_init)                                   \
  private:                                                                                         \
    struct {                                                                                       \
        v_type value = v_init;                                                                     \
        std::mutex mtx;                                                                            \
        std::condition_variable cv;                                                                \
    } m_state_##name;                                                                              \
                                                                                                   \
  public:                                                                                          \
    void set_##name(v_type v, bool notify_on_false = false) {                                      \
        {                                                                                          \
            std::lock_guard<std::mutex> lock(m_state_##name.mtx);                                  \
            m_state_##name.value = v;                                                              \
        }                                                                                          \
        if (v || notify_on_false) {                                                                \
            m_state_##name.cv.notify_all();                                                        \
        }                                                                                          \
    };                                                                                             \
    v_type get_##name() {                                                                          \
        v_type res = v_init;                                                                       \
        {                                                                                          \
            std::lock_guard<std::mutex> lock(m_state_##name.mtx);                                  \
            res = m_state_##name.value;                                                            \
        }                                                                                          \
        return res;                                                                                \
    };                                                                                             \
    void wait_for_##name(const int wait_ms = 0) {                                                  \
        std::unique_lock<std::mutex> lock(m_state_##name.mtx);                                     \
        while (!m_state_##name.value) {                                                            \
            if (wait_ms > 0) {                                                                     \
                auto res = m_state_##name.cv.wait_for(lock, std::chrono::milliseconds(wait_ms));   \
                if (res == std::cv_status::timeout) {                                              \
                    return;                                                                        \
                }                                                                                  \
            } else {                                                                               \
                m_state_##name.cv.wait(lock);                                                      \
            }                                                                                      \
        }                                                                                          \
    }

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
                if (this->get_thread_alive()) {
                    {
                        std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                        if (m_on_thread_exit.func) {
                            m_on_thread_exit.func(m_on_thread_exit.ctx);
                        }
                        m_on_thread_exit.func = nullptr;
                        m_on_thread_exit.ctx = nullptr;
                    }
                    this->set_thread_alive(false);
                }
            };

            void run(ProcessWorkerContext ctx) noexcept(false) {
                if (this->get_thread_alive()) {
                    throw std::runtime_error("Process already exists");
                }
                std::thread(&ProcessWorker::process_thread, ctx, this).detach();
            };

            void set_on_thread_exit(std::function<void(void*)> on_thread_exit, void* ctx) {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    m_on_thread_exit.func = on_thread_exit;
                    m_on_thread_exit.ctx = ctx;
                }
                return;
            }

            std::string pass_through(const std::string& req_str);

          private:
            static void process_thread(ProcessWorkerContext ctx, ProcessWorker* worker);

            static void exit_thread(ExitContext& exit_ctx);

            AK_DEF_KERNEL_PROCESS_WORKER_STATE(thread_alive, bool, false);

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
        };

    }
}
