#pragma once

#include <libakcore/rational.h>
#include <libakcore/audio.h>
#include <libakcore/memory.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace akashi {

    namespace state {
        class AKState;
    }
    namespace buffer {

        class AVBufferData;

        struct AudioQueueData {
            std::deque<std::unique_ptr<AVBufferData>> buf;
            size_t buf_offset = 0;

            // if true, when dequeuing for audio processing,
            // you have to start take buf from `buf_offset` defined in this struct
            bool on_process = false;
        };

        class AudioQueue final {
          public:
            using uuid_t = std::string;

          public:
            explicit AudioQueue(core::borrowed_ptr<state::AKState> state);
            virtual ~AudioQueue();

            size_t enqueue(uuid_t layer_uuid, std::unique_ptr<AVBufferData> buf_data);

            bool empty(uuid_t layer_uuid);

            const AVBufferData& front(uuid_t layer_uuid, bool check_empty);

            bool seek(const core::Rational& seek_pts);

            AudioQueueData& queue(uuid_t layer_uuid);

            size_t pop_front(uuid_t layer_uuid);

            void clear(bool notify = true);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            std::unordered_map<std::string, AudioQueueData> m_qmap;
            std::atomic<size_t> m_queue_size{0};
            unsigned int m_max_queue_size = 0;
        };

    }
}
