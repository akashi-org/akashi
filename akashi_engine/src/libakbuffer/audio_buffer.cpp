#include "./audio_buffer.h"
#include "./avbuffer.h"

#include <libakcore/rational.h>
#include <libakcore/audio.h>
#include <libakcore/logger.h>

#include <cstdlib>
#include <cstdint>
#include <memory>

using namespace akashi::core;

namespace akashi {
    namespace buffer {

        // We assume that each channel has its own AudioRingBuffer
        class AudioSequence final {
          public:
            explicit AudioSequence(const size_t max_buf_size, const core::AKAudioSpec& audio_spec) {
                m_max_buf_size = max_buf_size;

                m_audio_spec = audio_spec;
                // force FLTP format
                m_audio_spec.format = core::AKAudioSampleFormat::FLTP;

                m_bytes_per_sample = core::size_table(m_audio_spec.format);

                m_buf_length = (m_max_buf_size / m_bytes_per_sample);

                m_buffer = std::make_unique<uint8_t[]>(m_buf_length);
            }
            virtual ~AudioSequence() {}

            bool write(const uint8_t* w_buf, const size_t w_buf_length, const core::Rational& w_pts,
                       double gain, bool mix = false) {
                if (!this->within_range(w_buf_length, w_pts)) {
                    return false;
                }
                auto w_offset = this->to_length(w_pts - m_buf_pts);
                auto write_idx = (m_read_idx + w_offset) % m_buf_length;

                if (write_idx % 4 != 0) {
                    AKLOG_DEBUGN("write_idx is not divisible by 4. Using an aligned one instead");
                    write_idx = 4 * ((write_idx / 4) + 1);
                }

                if (!mix) {
                    for (size_t i = 0; i < w_buf_length; i++) {
                        m_buffer[(write_idx + i) % m_buf_length] = w_buf[i];
                    }
                } else {
                    this->mix_layer(write_idx, w_buf, w_buf_length, gain);
                }
                return true;
            }

            bool read(uint8_t* r_buf, const size_t r_buf_length) const {
                if (r_buf_length > m_buf_length) {
                    return false;
                }
                for (size_t i = 0; i < r_buf_length; i++) {
                    r_buf[i] = m_buffer[(m_read_idx + i) % m_buf_length];
                }
                return true;
            }

            bool seek(const size_t r_buf_length) {
                // [TODO] do we really need this?
                // if (r_buf_length > m_buf_length) {
                //     return false;
                // }
                for (size_t i = 0; i < r_buf_length; i++) {
                    m_buffer[(m_read_idx + i) % m_buf_length] = 0;
                }
                m_read_idx = (m_read_idx + r_buf_length) % m_buf_length;
                m_buf_pts += this->to_pts(r_buf_length);
                return true;
            }

            bool seek(const core::Rational& dst_pts) {
                if ((dst_pts - m_buf_pts) < core::Rational(0l)) {
                    AKLOG_ERRORN("only forward seek is available!");
                    return false;
                }
                return this->seek(this->to_length(dst_pts - m_buf_pts));
            }

            core::Rational buf_pts(void) const { return m_buf_pts; }

            core::Rational max_buf_pts(void) const {
                return m_buf_pts + this->to_pts(m_buf_length);
            }

            core::Rational to_pts(const size_t buf_length) const {
                return (core::Rational(buf_length, 1) / this->bytes_per_second2(m_audio_spec));
            }

            int64_t to_length(const core::Rational& pts) const {
                auto res = pts * this->bytes_per_second2(m_audio_spec);
                if (res.den() != 1) {
                    AKLOG_WARNN("result is not integer");
                    return res.to_decimal();
                }
                return res.num();
            }

            bool within_range(const size_t w_buf_length, const core::Rational& w_pts) const {
                if (w_pts < Rational(0l)) {
                    AKLOG_WARNN("w_pts is lower than 0!");
                    return false;
                }
                // lower bound
                if (w_pts < m_buf_pts) {
                    return false;
                }
                // upper bound
                if (w_pts + this->to_pts(w_buf_length) > this->max_buf_pts()) {
                    return false;
                }
                return true;
            }

          private:
            // per channel
            int64_t bytes_per_second2(const core::AKAudioSpec& spec) const {
                return spec.sample_rate * size_table(spec.format);
            }

            static float parse_float(const uint8_t* buf) {
                float value = 0.0f;
                memcpy(&value, buf, sizeof(float));
                return value;
            }

            void mix_layer(const size_t write_idx, const uint8_t* w_buf, const size_t w_buf_length,
                           double gain) {
                alignas(float) uint8_t temp_buf[sizeof(float)] = {0};
                for (size_t i = 0; i < w_buf_length; i += sizeof(float)) {
                    for (size_t d = 0; d < sizeof(float); d++) {
                        temp_buf[d] = m_buffer[(i + d + write_idx) % m_buf_length];
                    }
                    float old_v = parse_float(temp_buf);
                    float new_v = parse_float(&w_buf[i]) * gain;
                    float mix_gain = 1.0;
                    float res = old_v + (mix_gain * new_v);

                    for (size_t j = 0; j < sizeof(float); j++) {
                        m_buffer[((i + write_idx) + j) % m_buf_length] = ((uint8_t*)&res)[j];
                    }
                }
            }

          private:
            std::unique_ptr<uint8_t[]> m_buffer = nullptr;
            core::Rational m_buf_pts = core::Rational(0, 1);
            core::AKAudioSpec m_audio_spec;
            size_t m_buf_length = 0;
            size_t m_max_buf_size = 0;
            size_t m_bytes_per_sample = 0;
            size_t m_read_idx = 0;
        };

        AudioBuffer::AudioBuffer(const core::AKAudioSpec& spec, const size_t max_bufsize) {
            for (int i = 0; i < spec.channels; i++) {
                m_buffers.push_back(make_owned<AudioSequence>(max_bufsize / spec.channels, spec));
            }
        };

        AudioBuffer::~AudioBuffer(){};

        AudioBuffer::Result AudioBuffer::enqueue(core::owned_ptr<AVBufferData> buf_data) {
            if (m_back_buffer) {
                auto back_buffer = std::move(m_back_buffer);
                m_back_buffer = nullptr;
                // [XXX] here enqueue() must not be called when m_back_buffer remains filled
                auto res = this->enqueue(std::move(back_buffer));
                // this result is unexpected, but we handle it just in case
                if (res != AudioBuffer::Result::OK) {
                    AKLOG_ERROR("Got invalid ResultCode {}", res);
                    return AudioBuffer::Result::ERR;
                }
            }

#ifndef NDEBUG
            if (buf_data->prop().sample_format != core::AKAudioSampleFormat::FLTP) {
                AKLOG_ERROR("sample format is not FLTP, {}", buf_data->prop().sample_format);
                return AudioBuffer::Result::ERR;
            }
#endif
            auto wbufs = this->to_buffers(*buf_data);

            for (size_t i = 0; i < wbufs.size(); i++) {
                if (!m_buffers[i]->write(wbufs[i].buf, wbufs[i].len, buf_data->prop().pts,
                                         buf_data->prop().gain, true)) {
                    m_back_buffer = std::move(buf_data);
                    return AudioBuffer::Result::OUT_OF_RANGE;
                }
            }
            return AudioBuffer::Result::OK;
        }

        AudioBuffer::Result AudioBuffer::dequeue(uint8_t* buf, const size_t len) {
            auto nb_channels = m_buffers.size();
            for (size_t i = 0; i < nb_channels; i++) {
                auto len_per_ch = len / nb_channels;
                auto offset = i * (len_per_ch);
                if (!m_buffers[i]->read(&buf[offset], len_per_ch)) {
                    return AudioBuffer::Result::OUT_OF_RANGE;
                }
                if (!m_buffers[i]->seek(len_per_ch)) {
                    return AudioBuffer::Result::ERR;
                }
            }

            return AudioBuffer::Result::OK;
        }

        bool AudioBuffer::seek(const size_t byte_size) {
            for (auto&& buffer : m_buffers) {
                if (!buffer->seek(byte_size)) {
                    // [TODO] maybe we should add some rollback for this?
                    return false;
                }
            }
            return true;
        }

        bool AudioBuffer::write_ready() const {
            if (!m_back_buffer) {
                return true;
            }
            if (m_buffers.empty()) {
                return false;
            }
            auto wbuf_size = m_back_buffer->prop().data_size / m_back_buffer->prop().channels;
            auto wpts = m_back_buffer->prop().pts;
            return this->m_buffers[0]->within_range(wbuf_size, wpts);
        }

        core::Rational AudioBuffer::cur_pts() const {
            if (m_buffers.empty()) {
                return core::Rational(-1, 1);
            }
            return m_buffers[0]->buf_pts();
        }

        std::vector<AudioBuffer::Data> AudioBuffer::to_buffers(const AVBufferData& buf_data) const {
            std::vector<AudioBuffer::Data> res_data;
            auto nb_channels = buf_data.prop().channels;
            for (int i = 0; i < nb_channels; i++) {
                AudioBuffer::Data data;
                data.len = buf_data.prop().data_size / nb_channels;
                data.buf = buf_data.prop().audio_data[i];
                res_data.push_back(std::move(data));
            }
            return res_data;
        }

    }
}
