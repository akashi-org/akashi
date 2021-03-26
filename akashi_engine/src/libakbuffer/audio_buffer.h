#pragma once

#include <libakcore/rational.h>
#include <libakcore/audio.h>
#include <libakcore/memory.h>

#include <vector>

namespace akashi {
    namespace buffer {

        class AVBufferData;
        class AudioSequence;

        class AudioBuffer final {
          public:
            struct Data {
                uint8_t* buf = nullptr;
                size_t len = 0;
            };
            enum class Result {
                OUT_OF_RANGE = -2,
                ERR = -1,
                NONE = 0,
                OK = 1,
            };

          public:
            explicit AudioBuffer(const core::AKAudioSpec& spec, const size_t max_bufsize);

            virtual ~AudioBuffer();

            // if failed, input buffer is to be stored in m_back_buffer
            // precondition: write_ready() returns true
            AudioBuffer::Result enqueue(core::owned_ptr<AVBufferData> buf_data);

            /**
             *
             * @params (buf) a 1-D planar audio buffer
             * @params (buf_size) buf length, not in bytes
             *
             * @detail
             * precondition: `buf` is properly allocated
             */
            AudioBuffer::Result dequeue(uint8_t* buf, const size_t len);

            bool seek(const size_t byte_size);

            bool write_ready() const;

            core::Rational cur_pts() const;

          private:
            std::vector<AudioBuffer::Data> to_buffers(const AVBufferData& buf_data) const;

          private:
            core::owned_ptr<AVBufferData> m_back_buffer = nullptr;
            std::vector<core::owned_ptr<AudioSequence>> m_buffers;
        };

    }
}
