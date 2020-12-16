#pragma once

#include <libakcore/rational.h>
#include <libakcore/audio.h>
#include <libakcore/memory.h>

namespace akashi {

    namespace state {
        class AKState;
    }

    namespace buffer {

        enum class AVBufferType { UNKNOWN = -1, VIDEO = 0, AUDIO };

        class AVBufferData {
          public:
            struct Property {
                struct VideoEntry {
                    uint8_t* buf = nullptr;
                    int stride = -1;
                };

                AVBufferType media_type = AVBufferType::UNKNOWN;
                core::Rational pts = core::Rational(-1, 1);
                core::Rational rpts = core::Rational(-1, 1);
                bool start_frame = false;
                const char* uuid = "";
                VideoEntry video_data[3];
                uint8_t* audio_data = nullptr;
                size_t data_size = 0;
                int chroma_width = -1;
                int chroma_height = -1;
                int width = -1;
                int height = -1;
                int sample_rate = -1;
                core::AKAudioSampleFormat sample_format = core::AKAudioSampleFormat::NONE;
                int channels = -1;
                int nb_samples = -1;
            };

          public:
            virtual ~AVBufferData(){};
            virtual const Property& prop(void) const { return m_prop; };
            virtual bool is_dummy() const { return false; }

          protected:
            Property m_prop;
        };

        class DummyBufferData : public AVBufferData {
          public:
            bool is_dummy() const override { return true; }
        };

        class VideoQueue;
        class AudioQueue;
        class AVBuffer final {
          public:
            core::owned_ptr<VideoQueue> vq;
            core::owned_ptr<AudioQueue> aq;

          public:
            explicit AVBuffer(core::borrowed_ptr<state::AKState> state);
            virtual ~AVBuffer() = default;
        };

    }
}
