#pragma once

#include <mutex>
#include <condition_variable>

#define AK_DEF_SYNC_ENCODE_STATE(name, v_type, v_init)                                             \
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
    }                                                                                              \
    void wait_for_not_##name() {                                                                   \
        std::unique_lock<std::mutex> lock(m_state_##name.mtx);                                     \
        while (m_state_##name.value) {                                                             \
            m_state_##name.cv.wait(lock);                                                          \
        }                                                                                          \
    }

namespace akashi {
    namespace encoder {

        struct EncodeStateProp {};

        class EncodeState final {
            AK_DEF_SYNC_ENCODE_STATE(producer_finished, bool, false);
            AK_DEF_SYNC_ENCODE_STATE(consumer_finished, bool, false);

          public:
            explicit EncodeState(void);
            virtual ~EncodeState(void);

          public:
            EncodeStateProp m_prop;
            std::mutex m_prop_mtx;
        };

    }
}
