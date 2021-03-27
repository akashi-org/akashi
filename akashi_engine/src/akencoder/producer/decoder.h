#pragma once

#include <libakcore/memory.h>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace codec {
        class AKDecoder;
    }
    namespace buffer {
        class AVBuffer;
        class AudioBuffer;
    }
    namespace encoder {

        struct DecodeParams {
            core::borrowed_ptr<state::AKState> state;
            core::borrowed_ptr<codec::AKDecoder> decoder;
            core::borrowed_ptr<buffer::AVBuffer> buffer;
            core::borrowed_ptr<buffer::AudioBuffer> abuffer;
        };

        enum class DecodeResult { ERR = -1, ENDED = 0, OK = 1 };

        DecodeResult exec_decode(DecodeParams& decode_params);
    }
}
