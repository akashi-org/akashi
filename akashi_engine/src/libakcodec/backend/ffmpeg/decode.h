#pragma once

#include <cstdint>
#include <functional>

extern "C" {
#include <libavutil/rational.h>
}

struct AVPacket;
struct AVFrame;
struct AVStream;
struct AVCodecContext;

namespace akashi {
    namespace codec {

        struct InputSource;
        struct DecodeStream;

        int decode_packet(AVPacket* pkt, AVFrame* frame, AVCodecContext* dec_ctx);

    }
}
