#pragma once

#include <libakcore/rational.h>
#include <libakcore/audio.h>
#include <libakcore/memory.h>
#include <libakcore/hw_accel.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace core {
        struct AtomProfile;
    }
    namespace buffer {

        class AVBufferData;

        struct VideoQueueData {
            std::deque<std::unique_ptr<AVBufferData>> buf;
        };

        class VideoQueue final {
          public:
            using uuid_t = std::string;

          public:
            explicit VideoQueue(core::borrowed_ptr<state::AKState> state);
            virtual ~VideoQueue();

            size_t enqueue(uuid_t layer_uuid, std::unique_ptr<AVBufferData> buf_data);

            std::unique_ptr<AVBufferData> dequeue(uuid_t layer_uuid, const core::Rational& pts);

            bool seek(const core::Rational& seek_pts);

            void clear(bool notify = true);

            void clear_by_id(uuid_t layer_uuid);

          private:
            bool is_not_full(void) const;

          private:
            core::borrowed_ptr<state::AKState> m_state;
            std::unordered_map<std::string, VideoQueueData> m_qmap;
            size_t m_queue_size = 0;
            size_t m_queue_count = 0; // queue count in total
            std::mutex m_qmap_mtx;
            unsigned int m_max_queue_size = 0;
            size_t m_max_queue_count = 0;
            core::VideoDecodeMethod m_decode_method = core::VideoDecodeMethod::NONE;
        };

    }
}
