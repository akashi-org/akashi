#include "./encoder.h"

#include "./backend/ffmpeg.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

using namespace akashi::core;

namespace akashi {
    namespace codec {

        AKEncoder::AKEncoder(core::borrowed_ptr<state::AKState> state) {
            m_frame_sink = make_owned<FFFrameSink>(state);
        }

        AKEncoder::~AKEncoder() {}

        bool AKEncoder::open(void) { return m_frame_sink->open(); }

        bool AKEncoder::close(void) {
            AKLOG_INFON("Now closing encoder...");
            return m_frame_sink->close();
            AKLOG_INFON("Successfully closed");
        }

        EncodeResultCode AKEncoder::send(const EncodeArg& encode_arg) {
            return m_frame_sink->send(encode_arg);
        }

        EncodeWriteResult AKEncoder::write(const EncodeWriteArg& write_arg) {
            return m_frame_sink->write(write_arg);
        }

        size_t AKEncoder::nb_samples_per_frame(void) {
            return m_frame_sink->nb_samples_per_frame();
        }

        core::AKAudioSampleFormat
        AKEncoder::validate_audio_format(const core::AKAudioSampleFormat& sample_format) {
            return m_frame_sink->validate_audio_format(sample_format);
        }

    }
}
