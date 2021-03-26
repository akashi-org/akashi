#pragma once

#include <libakcore/rational.h>
#include <libakcore/audio.h>

#include <cstdlib>
#include <cstdint>
#include <memory>
#include <cassert>

// temporary
#include <cstdio>

#include <stdexcept>

namespace akashi {
    namespace buffer {

        // We assume that each channel has its own AudioRingBuffer
        class AudioRingBuffer2 final {
          public:
            explicit AudioRingBuffer2(const size_t max_buf_size,
                                      const core::AKAudioSpec& audio_spec) {
                m_max_buf_size = max_buf_size;

                m_audio_spec = audio_spec;
                // force FLTP format
                m_audio_spec.format = core::AKAudioSampleFormat::FLTP;

                m_bytes_per_sample = core::size_table(m_audio_spec.format);

                m_buf_length = (m_max_buf_size / m_bytes_per_sample);

                m_buffer = std::make_unique<uint8_t[]>(m_buf_length);
            }
            virtual ~AudioRingBuffer2() {}

            bool write(const uint8_t* w_buf, const size_t w_buf_length, const core::Rational& w_pts,
                       bool mix = false) {
                if (!this->within_range(w_buf_length, w_pts)) {
                    return false;
                }
                auto w_offset = this->to_length(w_pts - m_buf_pts);
                auto write_idx = (m_read_idx + w_offset) % m_buf_length;

                if (write_idx % 4 != 0) {
                    fprintf(stderr,
                            "write_idx is not divisible by 4. Using an aligned one instead.");
                    write_idx = 4 * ((write_idx / 4) + 1);
                }

                if (!mix) {
                    for (size_t i = 0; i < w_buf_length; i++) {
                        m_buffer[(write_idx + i) % m_buf_length] = w_buf[i];
                    }
                } else {
                    this->mix_layer(write_idx, w_buf, w_buf_length);
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

            // [TODO] we should remove this method later
            bool seek2(const size_t byte_size) { return this->seek(byte_size); }

            bool seek(const core::Rational& dst_pts) {
                if ((dst_pts - m_buf_pts) < core::Rational(0l)) {
                    fprintf(stderr, "seek(): only forward seek is available!\n");
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
                    fprintf(stderr, "to_length(): result is not integer!\n");
                    return res.to_decimal();
                }
                return res.num();
            }

            void mix_layer(const size_t write_idx, const uint8_t* w_buf,
                           const size_t w_buf_length) {
                for (size_t i = 0; i < w_buf_length; i += 4) {
                    float old_v = *(float*)(&m_buffer[(i + write_idx) % m_buf_length]);
                    // float new_v = *(float*)(&audio_data[i]) * layer.gain;
                    float new_v = *(float*)(&w_buf[i]) * 1;
                    float mix_gain = 1.0;
                    float res = old_v + (mix_gain * new_v);

                    for (size_t j = 0; j < sizeof(float); j++) {
                        m_buffer[((i + write_idx) + j) % m_buf_length] = ((uint8_t*)&res)[j];
                    }
                }
            }

            bool within_range(const size_t w_buf_length, const core::Rational& w_pts) const {
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

            // debug
            const uint8_t* raw() { return m_buffer.get(); }

          private:
            // per channel
            int64_t bytes_per_second2(const core::AKAudioSpec& spec) const {
                return spec.sample_rate * size_table(spec.format);
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

        // We assume that each channel has its own AudioRingBuffer
        class AudioRingBuffer final {
          public:
            explicit AudioRingBuffer(const size_t max_buf_size,
                                     const core::AKAudioSpec& audio_spec) {
                m_max_buf_size = max_buf_size;

                m_audio_spec = audio_spec;
                // force FLTP format
                m_audio_spec.format = core::AKAudioSampleFormat::FLTP;

                m_bytes_per_sample = core::size_table(m_audio_spec.format);

                m_buf_length = (m_max_buf_size / m_bytes_per_sample);

                m_buffer = std::make_unique<float[]>(m_buf_length);
            }
            virtual ~AudioRingBuffer() {}

            bool write(const float* w_buf, const size_t w_buf_length, const core::Rational& w_pts,
                       bool mix = false) {
                if (!this->within_range(w_buf_length, w_pts)) {
                    return false;
                }
                auto w_offset = this->to_length(w_pts - m_buf_pts);

                if (!mix) {
                    for (size_t i = 0; i < w_buf_length; i++) {
                        m_buffer[(m_read_idx + w_offset + i) % m_buf_length] = w_buf[i];
                    }
                } else {
                    this->mix_layer((m_read_idx + w_offset) % m_buf_length, w_buf, w_buf_length);
                }
                return true;
            }

            bool read(float* r_buf, const size_t r_buf_length) const {
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
                m_read_idx = (m_read_idx + r_buf_length) % m_buf_length;
                m_buf_pts += this->to_pts(r_buf_length);
                return true;
            }

            bool seek2(const size_t byte_size) {
                auto buf_length =
                    core::Rational(byte_size, 1) / core::Rational(m_bytes_per_sample, 1);
                if (buf_length.den() != 1) {
                    fprintf(stderr, "seek2(): byte_size is not divisible by %ld!\n",
                            m_bytes_per_sample);
                    return false;
                }
                return this->seek(buf_length.num());
            }

            bool seek(const core::Rational& dst_pts) {
                if ((dst_pts - m_buf_pts) < core::Rational(0l)) {
                    fprintf(stderr, "seek(): only forward seek is available!\n");
                    return false;
                }
                return this->seek(this->to_length(dst_pts - m_buf_pts));
            }

            core::Rational buf_pts(void) const { return m_buf_pts; }

            core::Rational max_buf_pts(void) const {
                return m_buf_pts + this->to_pts(m_buf_length);
            }

            core::Rational to_pts(const size_t buf_length) const {
                return (core::Rational(buf_length * sizeof(float), 1) /
                        this->bytes_per_second2(m_audio_spec));
            }

            int64_t to_length(const core::Rational& pts) const {
                auto res = (pts * this->bytes_per_second2(m_audio_spec)) /
                           core::Rational(sizeof(float), 1);
                if (res.den() != 1) {
                    fprintf(stderr, "to_length(): result is not integer!\n");
                    return res.to_decimal();
                }
                return res.num();
            }

            void mix_layer(const size_t write_idx, const float* w_buf, const size_t w_buf_length) {
                for (size_t i = 0; i < w_buf_length; i++) {
                    // float new_v = *(float*)(&audio_data[i]) * layer.gain;
                    float mix_gain = 1.0;
                    float res = m_buffer[(write_idx + i) % m_buf_length] + (mix_gain * w_buf[i]);
                    m_buffer[(write_idx + i) % m_buf_length] = res;
                }
            }

            bool within_range(const size_t w_buf_length, const core::Rational& w_pts) const {
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

            // debug
            const float* raw() { return m_buffer.get(); }

          private:
            // per channel
            int64_t bytes_per_second2(const core::AKAudioSpec& spec) const {
                return spec.sample_rate * size_table(spec.format);
            }

          private:
            std::unique_ptr<float[]> m_buffer = nullptr;
            core::Rational m_buf_pts = core::Rational(0, 1);
            core::AKAudioSpec m_audio_spec;
            size_t m_buf_length = 0;
            size_t m_max_buf_size = 0;
            size_t m_bytes_per_sample = 0;
            size_t m_read_idx = 0;
        };

    }
}
