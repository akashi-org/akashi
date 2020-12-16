#include "./decode.h"
#include "./input.h"

#include <libakcore/logger.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include <cstdio>
#include <string>

using namespace akashi::core;

namespace akashi {
    namespace codec {

        // return 0 if success, -11 if EAGAIN.
        // If error occurs, other negative value will be returned.
        int decode_packet(AVPacket* pkt, AVFrame* frame, AVCodecContext* dec_ctx) {
            int ret_code = 0;

            if ((ret_code = avcodec_send_packet(dec_ctx, pkt)) < 0) {
                AKLOG_ERROR("decode_packet(): avcodec_send_packet error. ret={}",
                            AVERROR(ret_code));
                goto exit;
            }

            if ((ret_code = avcodec_receive_frame(dec_ctx, frame)) < 0) {
                if (ret_code != AVERROR(EAGAIN)) {
                    AKLOG_ERROR("decode_packet(): avcodec_receive_frame error. ret={}",
                                AVERROR(ret_code));
                }
                goto exit;
            }

        exit:
            return ret_code;
        }

    }
}
