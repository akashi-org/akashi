#pragma once

#include <libakcore/memory.h>
#include <libakcodec/encode_item.h>
#include <vector>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace core {
        class Rational;
    }
    namespace buffer {
        class AudioBuffer;
    }
    namespace encoder {

        std::vector<codec::EncodeArg> render_audio(core::Rational* audio_encode_pts,
                                                   const core::borrowed_ptr<state::AKState> state,
                                                   core::borrowed_ptr<buffer::AudioBuffer> abuffer,
                                                   const size_t nb_samples_per_frame,
                                                   const core::Rational& max_pts);

        std::vector<codec::EncodeArg>
        render_null_audio(core::Rational* audio_encode_pts,
                          const core::borrowed_ptr<state::AKState> state,
                          const size_t nb_samples_per_frame, const core::Rational& max_pts);

    }
}
