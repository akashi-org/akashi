#include "./callback.h"
#include "./context.h"
#include "./util.h"
#include "./mixer.h"

#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>

#include <libakstate/akstate.h>
#include <libakplayer/event.h>
#include <libakcore/logger.h>
#include <libakcore/rational.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        void CallbackContext::destroy(void){};

        core::Rational CallbackContext::current_time(void) const {
            return m_audio_ctx->current_time();
        };

        const char* CallbackContext::get_pa_error(void) const {
            return m_audio_ctx->get_pa_error();
        };

        size_t CallbackContext::bytes_per_second(void) const {
            return m_audio_ctx->bytes_per_second();
        };

        core::Rational CallbackContext::to_pts(size_t bytes) const {
            return m_audio_ctx->to_pts(bytes);
        };

        bool CallbackContext::is_play_over(void) const {
            return m_state->m_atomic_state.current_atom_index.load() >=
                   this->atom_profiles().size() - 1;
        };

        void CallbackContext::incr_loop_cnt(void) {
            // [TODO] lock-free?
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.trigger_video_reset = true;
            }
            m_state->m_atomic_state.play_loop_cnt += 1;
            return;
        };

        size_t CallbackContext::loop_cnt(void) const {
            return m_state->m_atomic_state.play_loop_cnt;
        };

        void CallbackContext::incr_bytes_played(int64_t bytes) {
            m_state->m_atomic_state.bytes_played.fetch_add(bytes);
        }

        const std::vector<core::AtomProfile>& CallbackContext::atom_profiles(void) const {
            // [TODO] lock?
            return m_state->m_prop.render_prof.atom_profiles;
        };

        core::AtomProfile CallbackContext::select_current_atom(const size_t requested_bytes) {
            auto atom_profiles = this->atom_profiles();
            if (atom_profiles.empty()) {
                return {};
            }

            const auto cur_atom_to =
                atom_profiles[m_state->m_atomic_state.current_atom_index.load()].to;
            Rational next_pts = this->current_time() + this->to_pts(requested_bytes);

            if (next_pts <= cur_atom_to) {
                return atom_profiles[m_state->m_atomic_state.current_atom_index.load()];
            } else {
                if (this->is_play_over()) {
                    m_state->m_atomic_state.current_atom_index.store(0);
                    m_state->m_atomic_state.start_time.store(core::Rational{0, 1});
                    m_state->m_atomic_state.bytes_played.store(0);
                    this->incr_loop_cnt();
                } else {
                    m_state->m_atomic_state.current_atom_index.fetch_add(1);
                }
                return atom_profiles[m_state->m_atomic_state.current_atom_index.load()];
            }
        }

        state::PlayState CallbackContext::state(void) const {
            return m_state->m_atomic_state.audio_play_state.load();
        }

        double CallbackContext::volume(void) const { return m_state->m_atomic_state.volume.load(); }

        bool CallbackContext::is_buf_empty(std::string layer_uuid) {
            return m_buffer->aq->empty(layer_uuid);
        };

        const buffer::AVBufferData& CallbackContext::front_buf(std::string layer_uuid,
                                                               bool check_empty) {
            return m_buffer->aq->front(layer_uuid, check_empty);
        };

        bool CallbackContext::seek_buf(std::string layer_uuid, const core::Rational& seek_pts) {
            while (!this->is_buf_empty(layer_uuid)) {
                const auto buf_data = this->front_buf(layer_uuid, false);
                auto& cur_queue = this->queue(layer_uuid);
                const auto buf_from = buf_data.prop().pts + this->to_pts(cur_queue.buf_offset);
                const auto buf_to = buf_data.prop().pts + this->to_pts(buf_data.prop().data_size);
                if (buf_from <= seek_pts && seek_pts <= buf_to) {
                    const auto offset_pts = seek_pts - buf_from;
                    size_t offset_bytes = (offset_pts * this->bytes_per_second()).to_decimal();
                    cur_queue.on_process = offset_bytes > 0;
                    cur_queue.buf_offset = offset_bytes;

                    return true;
                }
                this->pop_front_buf(layer_uuid);
            }
            return false;
        };

        buffer::AudioQueueData& CallbackContext::queue(std::string layer_uuid) {
            return m_buffer->aq->queue(layer_uuid);
        };

        void CallbackContext::pop_front_buf(std::string layer_uuid) {
            auto queue_size = m_buffer->aq->pop_front(layer_uuid);
            bool can_play = queue_size >= MIN_PLAYABLE_QUEUE_SIZE;

            m_state->set_audio_play_ready(can_play);
            if (!can_play) {
                this->player_pause();
            }
        };

        void CallbackContext::player_pause(void) {
            if (m_event) {
                m_event->emit_change_play_state(state::PlayState::PAUSED);
            } else {
                AKLOG_WARNN("CallbackContext::player_pause(): m_event is null");
                return;
            }
        }

        /* callback */

        static struct {
            // [TODO] is it guaranteed that the sample size does not surpass the value of
            // MAX_AUDIO_BUFFER_SIZE?
            uint8_t buf[MAX_AUDIO_BUFFER_SIZE] = {0};
            size_t buf_size = 0;
        } s_mask_buf;

        static void fill_layer(uint8_t* buffer, const size_t buf_size, CallbackContext* cb_ctx,
                               const LayerProfile& layer, const size_t loop_cnt) {
            auto bytes_remaining = buf_size;
            auto buffer_offset = 0;
            const auto comp_layer_uuid = layer.uuid + std::to_string(loop_cnt);

            // seek in advance?
            // if (!(cb_ctx->seek_buf(track.src, cur_pts))) {
            //     AKLOG_ERRORN("fill_track() failed: seek failed");

            //     return;
            // }

            while (bytes_remaining > 0) {
                size_t bytes_to_fill = bytes_remaining;
                size_t audio_buf_size = 0;

                if (cb_ctx->is_buf_empty(comp_layer_uuid)) {
                    AKLOG_DEBUGN("fill_layer() failed: audio buffer not found.");
                    // cb_ctx->player_pause();
                    break;
                }

                const auto buf_data = cb_ctx->front_buf(comp_layer_uuid, false);
                auto& rbuf = cb_ctx->queue(comp_layer_uuid);
                if (rbuf.on_process) {
                    audio_buf_size = buf_data.prop().data_size - rbuf.buf_offset;
                } else {
                    audio_buf_size = buf_data.prop().data_size;
                }

                // [XXX] sample format for buf_data.prop().audio_data should always be interleaved
                // format in this case. So, we can assume that all sample data exists in the first
                // element of buffer.
                if (audio_buf_size <= bytes_remaining) {
                    bytes_to_fill = audio_buf_size;
                    mix_layer(&buffer[buffer_offset], bytes_to_fill,
                              &buf_data.prop().audio_data[0][rbuf.buf_offset], layer);
                    rbuf.buf_offset = 0;
                    rbuf.on_process = false;
                    cb_ctx->pop_front_buf(comp_layer_uuid);
                } else {
                    bytes_to_fill = bytes_remaining;
                    mix_layer(&buffer[buffer_offset], bytes_to_fill,
                              &buf_data.prop().audio_data[0][rbuf.buf_offset], layer);
                    rbuf.buf_offset += bytes_to_fill;
                    rbuf.on_process = true;
                }

                buffer_offset += bytes_to_fill;
                bytes_remaining -= bytes_to_fill;
            }
        }

        static bool has_overlap(const LayerProfile& track, const Rational& cur,
                                const Rational& next) {
            return track.from <= next && track.to >= cur;
        }

        void stream_write_cb(pa_stream* stream, size_t requested_bytes, void* userdata) {
            auto cb_ctx = (CallbackContext*)userdata;

            // [TODO] select_current_atom has sideeffects!!
            // loop_cnt,cur_pts,next_pts must be calculated after select_current_atom is called
            // clearly we need some refactoring for this
            const auto cur_atom = cb_ctx->select_current_atom(requested_bytes);
            const auto cur_layers = cur_atom.av_layers;

            if ((cb_ctx->state() != state::PlayState::PLAYING) || cur_layers.empty()) {
                // [TODO] we want just do a return, but in that case, callback will not be called
                // from then
                memset(s_mask_buf.buf, 0, requested_bytes);
                if (pa_stream_write(stream, s_mask_buf.buf, requested_bytes, NULL, 0,
                                    PA_SEEK_RELATIVE) < 0) {
                    AKLOG_ERROR("stream_write_cb() failed: {}", cb_ctx->get_pa_error());
                }
                cb_ctx->incr_bytes_played(requested_bytes);
                s_mask_buf.buf_size = 0;
                return;
            }

            Rational cur_pts = cb_ctx->current_time();
            Rational next_pts = cur_pts + cb_ctx->to_pts(requested_bytes);
            const size_t loop_cnt = cb_ctx->loop_cnt();

            memset(s_mask_buf.buf, 0, requested_bytes);
            s_mask_buf.buf_size = requested_bytes;

            // [TODO] need to handle the mutiple atoms

            // if (cb_ctx->atom_profiles().size() == 0) {
            //    logger::error<LogTag::PLAYER>(
            //        FORMAT_MSG("stream_write_cb() failed: atom_profiles is blank"));
            // }

            for (size_t i = 0; i < cur_layers.size(); i++) {
                if (has_overlap(cur_layers[i], cur_pts, next_pts)) {
                    const auto offset_pts = std::max(cur_layers[i].from - cur_pts, Rational(0, 1));
                    size_t offset_bytes = (offset_pts * cb_ctx->bytes_per_second()).to_decimal();

                    fill_layer(&s_mask_buf.buf[offset_bytes], requested_bytes - offset_bytes,
                               cb_ctx, cur_layers[i], loop_cnt);
                }
            }

            double rms = adjust_volume(s_mask_buf.buf, requested_bytes, cb_ctx->volume());
            if (std::isnan(rms)) {
                AKLOG_ERRORN("RMS is nan");
                memset(s_mask_buf.buf, 0, requested_bytes);
            }

            if (pa_stream_write(stream, s_mask_buf.buf, requested_bytes, NULL, 0,
                                PA_SEEK_RELATIVE) < 0) {
                AKLOG_ERROR("stream_write_cb() failed: {}", cb_ctx->get_pa_error());
            }

            cb_ctx->incr_bytes_played(requested_bytes);
            s_mask_buf.buf_size = 0;
        }

    }

}
