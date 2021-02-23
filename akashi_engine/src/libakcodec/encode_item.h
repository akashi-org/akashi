#pragma once

#include <libakcore/rational.h>
#include <libakbuffer/avbuffer.h>

#include <memory.h>

namespace akashi {
    namespace codec {
        struct EncodeArg {
            core::Rational pts = core::Rational(-1, 1);
            std::unique_ptr<uint8_t> buffer = nullptr;
            int buf_size = 0;
            buffer::AVBufferType type = buffer::AVBufferType::UNKNOWN;
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
