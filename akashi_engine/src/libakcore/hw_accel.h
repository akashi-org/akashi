#pragma once

namespace akashi {
    namespace core {
        enum class VideoDecodeMethod { NONE = -1, SW = 0, VAAPI, VAAPI_COPY };
    }
}
