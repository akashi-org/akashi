#pragma once

#include <libakcore/rational.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/hwframe.h>

#include <memory.h>

namespace akashi {
    namespace buffer {
        class HWFrame;
    }
    namespace codec {
        struct EncodeArg {
            core::Rational pts = core::Rational(-1, 1);
            std::unique_ptr<uint8_t[]> buffer = nullptr;
            std::unique_ptr<float[]> abuffer = nullptr;
            int buf_size = 0;
            size_t nb_samples = 0;
            size_t abuffer_len = 0;
            buffer::AVBufferType type = buffer::AVBufferType::UNKNOWN;
            std::unique_ptr<buffer::HWFrame> hwframe;
        };

        struct EncodeWriteArg {
            buffer::AVBufferType type = buffer::AVBufferType::UNKNOWN;
        };

        enum class EncodeResultCode {
            /* -100 ~ -1 -> encode failure */

            ERROR = -100,

            /* initial value */
            NONE = 0,

            /* 100 ~ 199 -> non-critical errors */

            SEND_EAGAIN = 100,
            RECV_EAGAIN = 101,
            RECV_EOF = 102,

            /* 200 ~ 299 -> encode successes */

            OK = 200,
        };

        struct EncodeWriteResult {
            EncodeResultCode result = EncodeResultCode::NONE;
        };

    }
}
