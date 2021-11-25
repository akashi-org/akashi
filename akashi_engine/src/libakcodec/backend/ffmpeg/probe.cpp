#include "../../probe.h"
#include "./error.h"

#include <cstdio>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

bool akprobe_get_duration(ak_fraction_t* duration, const char* url) {
    bool res = true;
    AVFormatContext* format_ctx = nullptr;

    av_log_set_level(AV_LOG_QUIET);

    if (auto ret = avformat_open_input(&format_ctx, url, nullptr, nullptr); ret < 0) {
        fprintf(stderr, "avformat_open_input() failed, code=%d(%s)\n", AVERROR(ret),
                akashi::codec::av_err2str(ret));
        res = false;
        goto exit;
    }

    if (auto ret = avformat_find_stream_info(format_ctx, nullptr); ret < 0) {
        fprintf(stderr, "avformat_find_stream_info() failed, code=%d(%s)\n", AVERROR(ret),
                akashi::codec::av_err2str(ret));
        res = false;
        goto exit;
    }

    if (format_ctx == nullptr) {
        fprintf(stderr, "failed to get avformat context");
        res = false;
        goto exit;
    }

    duration->num = format_ctx->duration;
    duration->den = AV_TIME_BASE;

exit:
    if (format_ctx) {
        avformat_close_input(&format_ctx);
        avformat_free_context(format_ctx);
    }

    return res;
}
