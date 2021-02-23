#pragma once

#include <libakcore/memory.h>
#include <libakcodec/encode_item.h>

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>

#define AK_DEF_ENCODE_QUEUE_STATE(name, v_type, v_init)                                            \
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
    void wait_for_##name() {                                                                       \
        std::unique_lock<std::mutex> lock(m_state_##name.mtx);                                     \
        while (!m_state_##name.value) {                                                            \
            m_state_##name.cv.wait(lock);                                                          \
        }                                                                                          \
    }

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace encoder {

        struct EncodeQueueData : public codec::EncodeArg {};

        class EncodeQueue final {
          public:
            inline const static EncodeQueueData BLANK_DATA = {};

          public:
            constexpr static size_t MAX_QUEUE_SIZE = 10;

          public:
            explicit EncodeQueue(core::borrowed_ptr<state::AKState> state);

            virtual ~EncodeQueue();

            void enqueue(EncodeQueueData&& data);

            EncodeQueueData dequeue(void);

            const EncodeQueueData& top(void);

            void clear(void);

            AK_DEF_ENCODE_QUEUE_STATE(not_empty, bool, false);
            AK_DEF_ENCODE_QUEUE_STATE(not_full, bool, true);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            struct {
                std::deque<EncodeQueueData> buf;
                std::mutex mtx;
            } m_synced_buf;
        };
    }
}
