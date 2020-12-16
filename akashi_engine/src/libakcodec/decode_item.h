#pragma once

#include <libakcore/audio.h>
#include <libakcore/memory.h>

#include <string>
#include <functional>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace buffer {
        class AVBufferData;
    }
    namespace codec {

        struct DecodeArg {
            core::AKAudioSpec out_audio_spec;
        };

        enum class DecodeResultCode {
            /* -100 ~ -1 -> decode failure */

            ERROR = -100,

            /* initial value */
            NONE = 0,

            /* 100 ~ 199 -> decode successes, but no buffer for some reason */

            DECODE_LAYER_EOF = 100, // when reached at eof of the targeted input source
            DECODE_LAYER_ENDED,  // decode for the targeted input source should be ended for reasons
                                 // like out of range, or etc.
            DECODE_STREAM_ENDED, // decode for one of the streams of the targeted input source
                                 // should be ended for reasons like out of range, or etc.
            DECODE_ATOM_ENDED,   //
            DECODE_ENDED,        // when reached at end of the video
            DECODE_AGAIN,
            DECODE_SKIPPED, // decode should be skipped for reasons like invalid pts, or etc.

            /* 200 ~ 299 -> decode successes, and got correct buffer */

            OK = 200,
        };

        struct DecodeResult {
            DecodeResultCode result = DecodeResultCode::NONE;
            core::owned_ptr<buffer::AVBufferData> buffer;
            const char* layer_uuid = ""; // [TODO] memory leak
            const char* atom_uuid = "";  // [TODO] memory leak
        };

    }
}
