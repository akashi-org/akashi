#include "./kernel_event.h"

#include <libakcore/memory.h>
#include <libakstate/akstate.h>
#include <libakcore/logger.h>

#include <mutex>
#include <deque>
#include <vector>

using namespace akashi::core;

namespace akashi {
    namespace kernel {

        KernelEventQueue::KernelEventQueue() {}

        KernelEventQueue::~KernelEventQueue() {
            m_state_not_empty.cv.notify_all();
            m_state_not_full.cv.notify_all();
        }

        void KernelEventQueue::enqueue(KernelEventQueueData&& data) {
            size_t buf_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                m_synced_buf.buf.push_back(std::move(data));
                buf_size = m_synced_buf.buf.size();
            }

            // if (buf_size >= m_max_queue_count) {
            //     this->set_not_full(false, true);
            // }

            this->set_not_empty(true, true);
        }

        KernelEventQueueData KernelEventQueue::dequeue(void) {
            KernelEventQueueData data;
            size_t buf_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                if (m_synced_buf.buf.empty()) {
                    return data;
                }
                data = std::move(m_synced_buf.buf.front());
                m_synced_buf.buf.pop_front();
                buf_size = m_synced_buf.buf.size();
            }

            // if (buf_size < m_max_queue_count) {
            //     this->set_not_full(true, true);
            // }
            if (buf_size <= 0) {
                this->set_not_empty(false, true);
            }
            return data;
        }

        void KernelEventQueue::clear(void) {
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                if (!m_synced_buf.buf.empty()) {
                    m_synced_buf.buf.clear();
                }
            }
            this->set_not_full(true, true);
            this->set_not_empty(false, true);
        }

        const KernelEventQueueData& KernelEventQueue::top(void) {
            std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
            if (m_synced_buf.buf.empty()) {
                return KernelEventQueue::BLANK_DATA;
            } else {
                return m_synced_buf.buf.front();
            }
        }

    }
}
