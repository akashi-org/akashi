#include "./video_queue.h"
#include "./avbuffer.h"

#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakcore/string.h>
#include <libakcore/memory.h>
#include <libakstate/akstate.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

using namespace akashi::core;

namespace akashi {

    namespace buffer {

        VideoQueue::VideoQueue(core::borrowed_ptr<state::AKState> state) : m_state(state) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_max_queue_size = m_state->m_prop.video_max_queue_size;
                m_max_queue_count = m_state->m_prop.video_max_queue_count;
                m_decode_method = m_state->m_atomic_state.preferred_decode_method.load();
            }
        }

        VideoQueue::~VideoQueue() {}

        size_t VideoQueue::enqueue(uuid_t layer_uuid, std::unique_ptr<AVBufferData> buf_data) {
            if (!buf_data) {
                AKLOG_ERRORN("VideoQueue::enqueue(): ownership lost");
                return 0;
            }

            bool not_full = true;
            size_t queue_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_qmap_mtx);

                m_queue_size += buf_data->prop().data_size;
                m_qmap[layer_uuid].buf.push_back(std::move(buf_data));

                queue_size = m_qmap.at(layer_uuid).buf.size();
                m_queue_count += 1;

                AKLOG_INFO("Enqueued {}, {}, id: {}", m_qmap.at(layer_uuid).buf.size(),
                           m_qmap.at(layer_uuid).buf.back()->prop().pts.to_decimal(),
                           layer_uuid.c_str());

                not_full = this->is_not_full();
            }
            m_state->set_video_decode_ready(not_full);

            // if (queue_size > 0) {
            //     Event evt;
            //     evt.name = EventName::UPDATE;
            //     this->emit_event(evt);
            //     logger::debug<LogTag::PLAYER>(
            //         FORMAT_MSG("PlayerContext::enqueue_rbuf(): Render Call"));
            // }

            return queue_size;
        };

        std::unique_ptr<AVBufferData> VideoQueue::dequeue(uuid_t layer_uuid,
                                                          const core::Rational& pts) {
            // should not be called when the queue is empty
            std::unique_ptr<AVBufferData> res(nullptr);
            bool not_full = true;
            {
                std::lock_guard<std::mutex> lock(m_qmap_mtx);
                bool contains = m_qmap.find(layer_uuid) != m_qmap.end();
                while (contains && !m_qmap[layer_uuid].buf.empty()) {
                    auto diff = m_qmap[layer_uuid].buf.front()->prop().pts - pts;

                    Rational drop_threshold = Rational(0, 1000);   // 0ms
                    Rational skip_threshold = Rational(100, 1000); // 10ms

                    // when the necessary pts is greater than that of the queue
                    if (diff < drop_threshold) {
                        AKLOG_INFO("🐬🐬🐬Dequeued dropped. Try to sync. {}, {}, diff: {}",
                                   m_qmap[layer_uuid].buf.size(), layer_uuid, diff.to_decimal());

                    }
                    // when the necessary pts is much smaller than that of the queue
                    else if (diff > skip_threshold) {
                        AKLOG_INFO("Dequeued skipped. Too early. {}, {}, {}, {}",
                                   m_qmap[layer_uuid].buf.size(), layer_uuid,
                                   m_qmap[layer_uuid].buf.front()->prop().pts.to_decimal(),
                                   pts.to_decimal());
                        break;
                    }
                    // when the necessary pts is a little smaller than or equal to that of the queue
                    else {
                        res = std::move(m_qmap[layer_uuid].buf.front());
                        m_queue_size -= res->prop().data_size;
                        m_queue_count -= 1;
                        m_qmap[layer_uuid].buf.pop_front();
                        AKLOG_INFO("Dequeued {}, {}, {}", m_qmap[layer_uuid].buf.size(),
                                   res->prop().pts.to_decimal(), layer_uuid);
                        break;
                    }

                    auto temp = std::move(m_qmap[layer_uuid].buf.front());
                    m_queue_size -= temp->prop().data_size;
                    m_queue_count -= 1;
                    m_qmap[layer_uuid].buf.pop_front();
                }
                not_full = this->is_not_full();
            }
            m_state->set_video_decode_ready(not_full);

            return res;
        };

        static bool validate_layer_uuid(const std::vector<std::string>& layer_uuids,
                                        const std::string& target_layer_uuid) {
            for (const auto& layer_uuid : layer_uuids) {
                if (layer_uuid == target_layer_uuid) {
                    return true;
                }
            }
            return false;
        }

        // [TODO] is it really ok to regard as a success case where only one layer which can be
        // used for seeking is found
        bool VideoQueue::seek(const core::Rational& seek_pts) {
            bool res = false;
            std::vector<std::string> layer_uuids;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                auto atom_profiles = m_state->m_prop.render_prof.atom_profiles;
                if (atom_profiles.size() == 0) {
                    AKLOG_ERRORN("AtomProfile not found");
                    return res;
                }
                for (const auto& layer : atom_profiles[0].av_layers) {
                    layer_uuids.push_back(layer.uuid);
                }
            }

            for (auto iter = m_qmap.begin(), end = m_qmap.end(); iter != end; ++iter) {
                const auto& target = *iter;

                if (!validate_layer_uuid(layer_uuids, target.first)) {
                    continue;
                }
                auto layer_uuid = target.first;
                bool not_full = true;
                {
                    std::lock_guard<std::mutex> lock(m_qmap_mtx);
                    bool contains = m_qmap.find(layer_uuid) != m_qmap.end();
                    while (contains && !m_qmap[layer_uuid].buf.empty()) {
                        auto diff = m_qmap[layer_uuid].buf.front()->prop().pts - seek_pts;

                        Rational seek_threshold = Rational(0, 1);

                        if (diff >= seek_threshold) {
                            AKLOG_INFO("Seeked want: {} , front: {}, uuid: {}",
                                       seek_pts.to_decimal(),
                                       m_qmap[layer_uuid].buf.front()->prop().pts.to_decimal(),
                                       layer_uuid);
                            res = true;
                            break;
                        }

                        auto temp = std::move(m_qmap[layer_uuid].buf.front());
                        m_queue_size -= temp->prop().data_size;
                        m_queue_count -= 1;
                        m_qmap[layer_uuid].buf.pop_front();
                    }
                    not_full = this->is_not_full();
                }
                m_state->set_video_decode_ready(not_full);
            }
            return res;
        }

        void VideoQueue::clear(bool notify) {
            {
                std::lock_guard<std::mutex> lock(m_qmap_mtx);
                m_qmap.clear();
                m_queue_size = 0;
                m_queue_count = 0;
            }
            if (notify) {
                m_state->set_video_decode_ready(true);
            }
        };

        void VideoQueue::clear_by_id(uuid_t layer_uuid) {
            bool not_full = true;
            {
                std::lock_guard<std::mutex> lock(m_qmap_mtx);
                bool contains = m_qmap.find(layer_uuid) != m_qmap.end();
                while (contains && !m_qmap[layer_uuid].buf.empty()) {
                    auto temp = std::move(m_qmap[layer_uuid].buf.front());
                    m_queue_size -= temp->prop().data_size;
                    m_queue_count -= 1;
                    m_qmap[layer_uuid].buf.pop_front();
                }
                not_full = this->is_not_full();
            }
            m_state->set_video_decode_ready(not_full);
        }

        bool VideoQueue::is_not_full(void) const {
            switch (m_decode_method) {
                case core::VideoDecodeMethod::VAAPI: {
                    return m_queue_count <= m_max_queue_count;
                }
                default: {
                    return m_queue_size <= m_max_queue_size;
                }
            }
        }

    }
}
