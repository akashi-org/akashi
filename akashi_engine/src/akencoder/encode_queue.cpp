#include "./encode_queue.h"

#include <libakcore/memory.h>
#include <libakstate/akstate.h>
#include <libakcore/logger.h>

#include <mutex>
#include <deque>
#include <vector>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        EncodeQueue::EncodeQueue(core::borrowed_ptr<state::AKState> state) : m_state(state) {
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                m_max_queue_count = m_state->m_encode_conf.encode_max_queue_count;
            }
        }

        EncodeQueue::~EncodeQueue() {
            m_state_not_empty.cv.notify_all();
            m_state_not_full.cv.notify_all();
        }

        void EncodeQueue::enqueue(EncodeQueueData&& data) {
            size_t buf_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                m_synced_buf.buf.push_back(std::move(data));
                buf_size = m_synced_buf.buf.size();
            }

            if (buf_size >= m_max_queue_count) {
                this->set_not_full(false, true);
            }

            this->set_not_empty(true, true);
        }

        EncodeQueueData EncodeQueue::dequeue(void) {
            EncodeQueueData data;
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

            if (buf_size < m_max_queue_count) {
                this->set_not_full(true, true);
            }
            if (buf_size <= 0) {
                this->set_not_empty(false, true);
            }
            return data;
        }

        void EncodeQueue::clear(void) {
            {
                std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
                if (!m_synced_buf.buf.empty()) {
                    m_synced_buf.buf.clear();
                }
            }
            this->set_not_full(true, true);
            this->set_not_empty(false, true);
        }

        const EncodeQueueData& EncodeQueue::top(void) {
            std::lock_guard<std::mutex> lock(m_synced_buf.mtx);
            if (m_synced_buf.buf.empty()) {
                return EncodeQueue::BLANK_DATA;
            } else {
                return m_synced_buf.buf.front();
            }
        }

    }
}