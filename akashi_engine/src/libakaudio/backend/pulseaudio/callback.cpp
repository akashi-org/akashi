#include "./callback.h"
#include "./context.h"
#include "./util.h"
#include "./mixer.h"

#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>

#include <libakstate/akstate.h>
#include <libakcore/logger.h>
#include <libakcore/rational.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        // [TODO] is it guaranteed that the sample size does not surpass the value of
        // MAX_AUDIO_BUFFER_SIZE?
        static uint8_t s_mask_buf[MAX_AUDIO_BUFFER_SIZE] = {0};

        static bool get_atom_profile(core::AtomProfile* atom_prof, const CallbackContext& cb_ctx) {
            const auto& atom_profiles = cb_ctx.atom_profiles();
            if (atom_profiles.empty()) {
                AKLOG_ERRORN("No atom found");
                return false;
            }
            *atom_prof = atom_profiles[0];
            return true;
        }

        static AudioProfile get_audio_profile(const core::AtomProfile& atom_prof,
                                              const CallbackContext& cb_ctx) {
            AudioProfile prof;
            prof.duration = atom_prof.duration;
            prof.bytes_per_second = cb_ctx.bytes_per_second();
            prof.current_time = cb_ctx.current_time();
            return prof;
        }

        size_t dur_to_bytes(const size_t& bytes_per_second, const Rational& duration) {
            return (duration * bytes_per_second).to_decimal();
        }

        Rational bytes_to_dur(const size_t& bytes_per_second, const size_t& bytes) {
            return Rational(bytes, bytes_per_second);
        }

        bool find_segment(WBSegment* segment, bool* is_play_over, const AudioProfile& prof,
                          uint8_t* mask_buf, size_t requested_bytes) {
            Rational p_from_pts = prof.current_time;
            if (p_from_pts < 0 || p_from_pts >= prof.duration) {
                AKLOG_ERROR("current time ({}) is out of range", p_from_pts.to_decimal());
                return false;
                // p_from_pts = 0;
            }

            const auto next_pts = p_from_pts + bytes_to_dur(prof.bytes_per_second, requested_bytes);
            Rational diff_pts = next_pts - prof.duration;
            Rational to_pts = diff_pts > 0 ? prof.duration : next_pts;

            WBSegment seg;
            seg.from_pts = p_from_pts;
            seg.to_pts = to_pts;
            seg.buf = mask_buf;
            seg.buf_size = dur_to_bytes(prof.bytes_per_second, (seg.to_pts - seg.from_pts));

            *segment = seg;
            *is_play_over = diff_pts > 0;

            return true;
        }

        static bool segment_has_overlap(const WBSegment& segment, const LayerProfile& track) {
            return track.from <= segment.to_pts && track.to >= segment.from_pts;
        }

        void segment_fill(const WBSegmentSlice& slice, core::borrowed_ptr<buffer::AudioQueue> aq,
                          const LayerProfile& layer) {
            auto layer_uuid = layer.uuid;
            if (aq->empty(layer_uuid)) {
                AKLOG_DEBUGN("No Audio buffer found");
                // cb_ctx->player_pause();
                return;
            }

            size_t aq_buf_size = 0;
            const auto buf_data = aq->front(layer_uuid, false);
            auto& rbuf = aq->queue(layer_uuid);

            if (rbuf.on_process) {
                aq_buf_size = buf_data.prop().data_size - rbuf.buf_offset;
            } else {
                aq_buf_size = buf_data.prop().data_size;
            }

            //  [XXX]
            //  Sample format for `buf_data.prop().audio_data` should always be interleaved
            //  format in this case. So, we can assume that all sample data exists in the first
            //  element of buffer.
            auto aq_buf = &buf_data.prop().audio_data[0][rbuf.buf_offset];
            auto bytes_to_fill = slice.buf_size <= aq_buf_size ? slice.buf_size : aq_buf_size;

            mix_layer(slice.buf, bytes_to_fill, aq_buf, layer);

            if (slice.buf_size >= aq_buf_size) {
                rbuf.buf_offset = 0;
                rbuf.on_process = false;
                aq->pop_front(layer_uuid);
            } else {
                rbuf.buf_offset += bytes_to_fill;
                rbuf.on_process = true;
            }

            if (slice.buf_size > aq_buf_size) {
                segment_fill(
                    {.buf = &slice.buf[bytes_to_fill], .buf_size = slice.buf_size - bytes_to_fill},
                    aq, layer);
            }
        }

        void stream_write_cb(pa_stream* stream, size_t requested_bytes, void* userdata) {
            memset(s_mask_buf, 0, requested_bytes);
            auto cb_ctx = (CallbackContext*)userdata;

            core::AtomProfile atom_prof;
            WBSegment segment;
            bool is_play_over = false;

            if ((cb_ctx->state() != state::PlayState::PLAYING)) {
                goto exit;
            }

            if (cb_ctx->audio_play_over()) {
                if (cb_ctx->video_play_over()) {
                    cb_ctx->player_pause();
                }
                goto exit;
            }

            if (!get_atom_profile(&atom_prof, *cb_ctx)) {
                goto exit;
            }

            // 1. segmentation
            {
                auto audio_prof = get_audio_profile(atom_prof, *cb_ctx);
                auto r =
                    find_segment(&segment, &is_play_over, audio_prof, s_mask_buf, requested_bytes);
                if (!r) {
                    AKLOG_ERRORN("collect_segments() failed");
                    goto exit;
                }
            }

            // 2. fill
            {
                for (const auto& cur_layer : atom_prof.av_layers) {
                    if (segment_has_overlap(segment, cur_layer)) {
                        segment_fill({.buf = segment.buf, .buf_size = segment.buf_size},
                                     cb_ctx->aq(), cur_layer);
                        cb_ctx->check_audio_play_ready();
                    }
                }
            }

            // 3. postproc
            {
                double rms = adjust_volume(s_mask_buf, requested_bytes, cb_ctx->volume());
                if (std::isnan(rms)) {
                    AKLOG_ERRORN("RMS is nan");
                    memset(s_mask_buf, 0, requested_bytes);
                }
            }

            // 4. update
            {
                cb_ctx->incr_bytes_played(segment.buf_size);
                if (is_play_over) {
                    cb_ctx->set_audio_play_over(true);
                }
            }

        exit:
            if (pa_stream_write(stream, s_mask_buf, requested_bytes, NULL, 0, PA_SEEK_RELATIVE) <
                0) {
                AKLOG_ERROR("stream_write_cb() failed: {}", cb_ctx->get_pa_error());
            }
        }

    }

}
