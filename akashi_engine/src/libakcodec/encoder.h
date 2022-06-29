#pragma once

#include "./encode_item.h"
#include <libakcore/audio.h>

#include <libakcore/memory.h>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace buffer {
        class HWFrame;
    }
    namespace codec {

        class FrameSink;
        class AKEncoder final {
          public:
            explicit AKEncoder(core::borrowed_ptr<state::AKState> state);
            virtual ~AKEncoder();

            bool open(void);

            bool close(void);

            EncodeResultCode send(const EncodeArg& encode_arg);

            EncodeWriteResult write(const EncodeWriteArg& write_arg);

            /*
             * number of audio samples (per channel) per frame
             */
            size_t nb_samples_per_frame(void);

            core::AKAudioSampleFormat
            validate_audio_format(const core::AKAudioSampleFormat& sample_format);

            std::unique_ptr<buffer::HWFrame> create_hwframe(void);

          private:
            core::owned_ptr<FrameSink> m_frame_sink;
        };

    }
}
