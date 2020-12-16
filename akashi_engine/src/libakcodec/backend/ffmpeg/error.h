#pragma once

extern "C" {
#include <libavutil/attributes.h>
#include <libavutil/error.h>
}
#include <cstring>

namespace akashi {
    namespace codec {

// In C++, av_err2str does not compile. (maybe it's just a matter of g++)
// Below is a workaround for this issue.
// ref: https://github.com/joncampbell123/composite-video-simulator/issues/5
// ref: https://ffmpeg.org/pipermail/libav-user/2013-January/003458.html
#ifdef av_err2str
#undef av_err2str
        av_always_inline char* av_err2str(int errnum) {
            static char str[AV_ERROR_MAX_STRING_SIZE];
            std::memset(str, 0, sizeof(str));
            return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
        }
#endif

    }

}
