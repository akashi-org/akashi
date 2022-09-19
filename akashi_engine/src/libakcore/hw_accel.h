#pragma once

namespace akashi {
    namespace core {
        enum class VideoDecodeMethod { NONE = -1, SW = 0, VAAPI, VAAPI_COPY };
        enum class VideoEncodeMethod { NONE = -1, SW = 0, VAAPI, VAAPI_COPY };

        namespace hwaccel {
            inline VideoDecodeMethod safe_map(VideoEncodeMethod method) {
                return static_cast<VideoDecodeMethod>(static_cast<int>(method));
            }
            inline VideoEncodeMethod safe_map(VideoDecodeMethod method) {
                return static_cast<VideoEncodeMethod>(static_cast<int>(method));
            }
        }
    }
}
