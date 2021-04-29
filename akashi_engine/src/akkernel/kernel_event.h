#pragma once

#include <libakserver/json_rpc.h>

#include <libakcore/memory.h>

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <chrono>
#include <variant>

#define AK_DEF_KERNEL_EVENT_QUEUE_STATE(name, v_type, v_init)                                      \
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

        enum class KernelEvent {
            INVALID = -1,
            ASP_REQUEST_RECEIVED = 101,
            PROCESS_EXIT = 201,
        };

        template <KernelEvent KE = KernelEvent::INVALID>
        struct KernelEventParams {};

        template <>
        struct KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED> {
            server::RPCRequest request;
            std::string req_str;
        };

        template <>
        struct KernelEventParams<KernelEvent::PROCESS_EXIT> {
            int exit_code;
        };

        using KernelEventParamsType =
            std::variant<KernelEventParams<>, KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED>,
                         KernelEventParams<KernelEvent::PROCESS_EXIT>>;

        struct KernelEventQueueData {
            KernelEvent evt;
            KernelEventParamsType params;
        };

        class KernelEventQueue final {
          public:
            inline const static KernelEventQueueData BLANK_DATA = {KernelEvent::INVALID, {}};

          public:
            explicit KernelEventQueue();

            virtual ~KernelEventQueue();

            void enqueue(KernelEventQueueData&& data);

            KernelEventQueueData dequeue(void);

            const KernelEventQueueData& top(void);

            void clear(void);

            AK_DEF_KERNEL_EVENT_QUEUE_STATE(not_empty, bool, false);
            AK_DEF_KERNEL_EVENT_QUEUE_STATE(not_full, bool, true);

          private:
            struct {
                std::deque<KernelEventQueueData> buf;
                std::mutex mtx;
            } m_synced_buf;
        };
    }
}
