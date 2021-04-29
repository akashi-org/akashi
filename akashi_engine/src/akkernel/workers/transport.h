#pragma once

#include <libakserver/json_rpc.h>

#include <libakcore/memory.h>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#define AK_DEF_TRANSPORT_LOOP_STATE(name, v_type, v_init)                                          \
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

        struct TransportWorkerContext {
            core::borrowed_ptr<KernelState> state;
            core::borrowed_ptr<KernelEventQueue> queue;
        };

        class TransportWorker final {
          private:
            struct ExitContext;

          public:
            explicit TransportWorker(){};

            virtual ~TransportWorker() { this->terminate(); }

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

            void run(TransportWorkerContext ctx) {
                std::thread(&TransportWorker::transport_thread, ctx, this).detach();
            };

            void set_on_thread_exit(std::function<void(void*)> on_thread_exit, void* ctx) {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    m_on_thread_exit.func = on_thread_exit;
                    m_on_thread_exit.ctx = ctx;
                }
                return;
            }

            AK_DEF_TRANSPORT_LOOP_STATE(asp_handled, bool, false);

            void set_asp_http_response(const server::HTTPRPCResponse& value) {
                {
                    std::lock_guard<std::mutex> lock(m_asp_http_resp.mtx);
                    m_asp_http_resp.value = value;
                }
            }

            server::HTTPRPCResponse asp_http_response(void) {
                {
                    std::lock_guard<std::mutex> lock(m_asp_http_resp.mtx);
                    return m_asp_http_resp.value;
                }
            }

          private:
            static void transport_thread(TransportWorkerContext ctx, TransportWorker* worker);

            static void exit_thread(ExitContext& exit_ctx);

          private:
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;
            bool m_thread_exited = false;

            struct {
                server::HTTPRPCResponse value;
                std::mutex mtx;
            } m_asp_http_resp;
        };

    }
}
