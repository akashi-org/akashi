#include "./audio_renderer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>

#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_buffer.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        std::vector<EncodeQueueData> render_audio(core::Rational* audio_encode_pts,
                                                  const core::borrowed_ptr<state::AKState> state,
                                                  core::borrowed_ptr<buffer::AudioBuffer> abuffer,
                                                  const size_t nb_samples_per_frame,
                                                  const core::Rational& max_pts) {
            std::vector<EncodeQueueData> datasets;

            auto audio_spec = state->m_atomic_state.audio_spec.load();
            auto audio_buf_size =
                nb_samples_per_frame * core::size_table(audio_spec.format) * audio_spec.channels;
            auto frame_dur = core::Rational(audio_buf_size, core::bytes_per_second(audio_spec));

            while (true) {
                auto frame_pts = *audio_encode_pts < Rational(0, 1) ? Rational(0, 1)
                                                                    : frame_dur + *audio_encode_pts;
                if (frame_pts > max_pts) {
                    break;
                }
                EncodeQueueData queue_data;
                queue_data.pts = frame_pts;
                queue_data.buf_size = audio_buf_size;
                queue_data.buffer.reset(new uint8_t[queue_data.buf_size]());
                queue_data.nb_samples = nb_samples_per_frame;
                queue_data.type = buffer::AVBufferType::AUDIO;

                auto result = abuffer->dequeue(queue_data.buffer.get(), queue_data.buf_size);
                if (result == buffer::AudioBuffer::Result::OUT_OF_RANGE) {
                    auto before_pts = abuffer->cur_pts();
                    abuffer->seek(queue_data.buf_size);
                    AKLOG_ERROR("AudioBuffer Seeked {} => {}", before_pts.to_decimal(),
                                abuffer->cur_pts().to_decimal());
                    continue;
                } else if (result != buffer::AudioBuffer::Result::OK) {
                    AKLOG_ERROR("Got invalid result {}", result);
                    // what should we do here?
                }

                AKLOG_INFO("AudioBuffer Dequeued, pts: {}", frame_pts.to_decimal());

                datasets.push_back(std::move(queue_data));

                *audio_encode_pts = frame_pts;
            }

            return datasets;
        }

        std::vector<EncodeQueueData>
        render_null_audio(core::Rational* audio_encode_pts,
                          const core::borrowed_ptr<state::AKState> state,
                          const size_t nb_samples_per_frame, const core::Rational& max_pts) {
            std::vector<EncodeQueueData> datasets;

            auto audio_spec = state->m_atomic_state.audio_spec.load();
            auto audio_buf_size =
                nb_samples_per_frame * core::size_table(audio_spec.format) * audio_spec.channels;
            auto frame_dur = core::Rational(audio_buf_size, core::bytes_per_second(audio_spec));

            while (true) {
                auto frame_pts = *audio_encode_pts < Rational(0, 1) ? Rational(0, 1)
                                                                    : frame_dur + *audio_encode_pts;
                if (frame_pts > max_pts) {
                    break;
                }
                EncodeQueueData queue_data;
                queue_data.pts = frame_pts;
                queue_data.buf_size = audio_buf_size;
                queue_data.buffer.reset(new uint8_t[queue_data.buf_size]());
                queue_data.nb_samples = nb_samples_per_frame;
                queue_data.type = buffer::AVBufferType::AUDIO;
                datasets.push_back(std::move(queue_data));

                *audio_encode_pts = frame_pts;
            }

            return datasets;
        }

#if 0
        static bool needs_finit = true;
        void save_pcm(uint8_t* buf, size_t buf_size, const char* prefix) {
            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "audio_%s.buf", prefix);

            FILE* f;

            if (needs_finit) {
                remove("audio_left.buf");
                remove("audio_right.buf");
                needs_finit = false;
            }

            f = fopen(frame_filename, "ab");
            fwrite(buf, 1, static_cast<size_t>(buf_size), f);
            fclose(f);
        };
#endif

    }

}
