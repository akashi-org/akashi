#pragma once

namespace akashi {
    namespace core {
        enum class EncodeCodec {
            NONE = -1,

            /* 100 ~ 199 -> video codecs */
            V_H264 = 100,

            /* 200 ~ 299 -> audio codecs */
            A_AAC = 200,
            A_MP3 = 201,
        };

    }
}
