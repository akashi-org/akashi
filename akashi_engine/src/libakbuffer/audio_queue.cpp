#include "./audio_queue.h"
#include "./avbuffer.h"

#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>
#include <libakcore/string.h>
#include <libakcore/audio.h>
#include <libakstate/akstate.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

using namespace akashi::core;

namespace akashi {

    namespace buffer {

        AudioQueue::AudioQueue(core::borrowed_ptr<state::AKState> state) : m_state(state) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_max_queue_size = m_state->m_prop.audio_max_queue_size;
            }
        }

        AudioQueue::~AudioQueue() {}

        size_t AudioQueue::enqueue(uuid_t layer_uuid, std::unique_ptr<AVBufferData> buf_data) {
            m_queue_size.fetch_add(buf_data->prop().data_size);

            auto last_item_pts = buf_data->prop().pts;
            auto last_item_rpts = buf_data->prop().rpts;

            m_qmap[layer_uuid].buf.push_back(std::move(buf_data));

            AKLOG_INFO("Audio buffer enqueued {}, {}, id: {}, rpts: {}",
                       m_qmap.at(layer_uuid).buf.size(), last_item_pts.to_decimal(),
                       layer_uuid.c_str(), last_item_rpts.to_decimal());

            size_t queue_size = m_queue_size.load();

            bool not_full = m_queue_size <= m_max_queue_size;
            m_state->set_audio_decode_ready(not_full);

            return queue_size;
        };

        bool AudioQueue::empty(uuid_t layer_uuid) {
            bool contains = m_qmap.find(layer_uuid) != m_qmap.end();
            return !contains || m_qmap[layer_uuid].buf.empty();
        };

        const static DummyBufferData dummy_buf_data{};

        const AVBufferData& AudioQueue::front(uuid_t layer_uuid, bool check_empty) {
            if (check_empty) {
                if (this->empty(layer_uuid)) {
                    // [TODO] error handling?
                    AKLOG_ERRORN("AudioContext::front_buf(): failed to get audio buffer");
                    return dummy_buf_data;
                }
            }
            return *m_qmap[layer_uuid].buf.front();
        };

        AudioQueueData& AudioQueue::queue(uuid_t layer_uuid) { return m_qmap[layer_uuid]; };

        size_t AudioQueue::pop_front(uuid_t layer_uuid) {
            // should not be called when the queue is empty

            bool contains = m_qmap.find(layer_uuid) != m_qmap.end();
            if (contains && !m_qmap[layer_uuid].buf.empty()) {
                auto res = std::move(m_qmap[layer_uuid].buf.front());
                m_queue_size.fetch_sub(res->prop().data_size);
                auto pop_pts = res->prop().pts.to_decimal();
                m_qmap[layer_uuid].buf.pop_front();
                AKLOG_INFO("audio buffer dequeued {}, {}, id: {}", m_qmap[layer_uuid].buf.size(),
                           pop_pts, layer_uuid.c_str());
            }

            size_t queue_size = m_queue_size.load();

            bool not_full = queue_size <= m_queue_size;
            m_state->set_audio_decode_ready(not_full);

            return queue_size;

            // if (!can_play) {
            //     this->player_pause();
            // }
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

        static int64_t bytes_per_second(core::borrowed_ptr<state::AKState> state) {
            auto audio_spec = state->m_atomic_state.audio_spec.load();
            return bytes_per_second(audio_spec);
        }

        static Rational to_pts(size_t bytes, core::borrowed_ptr<state::AKState> state) {
            return Rational(bytes, bytes_per_second(state));
        }

        // [TODO] is it really ok to regard as a success case where only one layer which can be
        // used for seeking is found
        bool AudioQueue::seek(const core::Rational& seek_pts) {
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
                bool initial_seek = true;
                while (!m_qmap[layer_uuid].buf.empty()) {
                    const auto& buf_data = m_qmap[layer_uuid].buf.front();
                    auto buf_from = buf_data->prop().pts;
                    if (initial_seek) {
                        buf_from += to_pts(m_qmap[layer_uuid].buf_offset, m_state);
                        initial_seek = false;
                    }
                    const auto buf_to =
                        buf_data->prop().pts + to_pts(buf_data->prop().data_size, m_state);

                    if (buf_from <= seek_pts && seek_pts <= buf_to) {
                        const auto offset_pts = seek_pts - buf_from;
                        size_t offset_bytes = (offset_pts * bytes_per_second(m_state)).to_decimal();
                        m_qmap[layer_uuid].on_process = offset_bytes > 0;
                        m_qmap[layer_uuid].buf_offset = offset_bytes;

                        AKLOG_INFO(
                            "AudioQueue::seek():  from: {}, to: {}, seek_pts: {}, offset_pts: {}, offset_bytes: {}",
                            buf_from.to_decimal(), buf_to.to_decimal(), seek_pts.to_decimal(),
                            offset_pts.to_decimal(), offset_bytes);
                        res = true;
                        break;
                    }
                    this->pop_front(layer_uuid);
                }
            }
            return res;
        }

        // [TODO] thread-unsafe
        void AudioQueue::clear(bool notify) {
            for (auto&& [k, v] : m_qmap) {
                v.buf.clear();
                v.buf_offset = 0;
                v.on_process = false;
            }
            m_queue_size.store(0);

            if (notify) {
                m_state->set_audio_decode_ready(true);
            }
        };

        void AudioQueue::clear_by_id(uuid_t layer_uuid) {
            bool contains = m_qmap.find(layer_uuid) != m_qmap.end();
            while (contains && !m_qmap[layer_uuid].buf.empty()) {
                auto res = std::move(m_qmap[layer_uuid].buf.front());
                m_queue_size.fetch_sub(res->prop().data_size);
                m_qmap[layer_uuid].buf.pop_front();
            }

            size_t queue_size = m_queue_size.load();

            bool not_full = queue_size <= m_queue_size;
            m_state->set_audio_decode_ready(not_full);
        }

        size_t AudioQueue::total_queue_size(void) { return m_queue_size.load(); }

        static void save_pcm(uint8_t* buf, size_t buf_size, const char* fname) {
            auto f = fopen(fname, "ab");
            fwrite(buf, 1, static_cast<size_t>(buf_size), f);
            fclose(f);
        };

        void AudioQueue::dump_all(void) {
            for (auto&& [k, v] : m_qmap) {
                auto fname = std::string(k + ".buf");
                remove(fname.c_str());
                for (size_t i = 0; i < v.buf.size(); i++) {
                    auto buf_size = v.buf[i]->prop().data_size;
                    auto buf_ptr = v.buf[i]->prop().audio_data[0];
                    save_pcm(buf_ptr, buf_size, fname.c_str());
                }
                AKLOG_WARN("Dumped pcm file: {}", fname.c_str());
            }
        }

    }
}
