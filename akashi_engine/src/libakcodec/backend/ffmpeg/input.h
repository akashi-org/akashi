#pragma once

#include <libakcore/rational.h>
#include <libakcore/hw_accel.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include <vector>

struct AVCodecContext;
struct AVFormatContext;
struct AVBufferRef;

namespace akashi {
    namespace codec {

        struct DecodeStream {
            AVCodecContext* dec_ctx = nullptr;
            struct SwrContext* swr_ctx = nullptr;
            bool swr_ctx_init_done = false;
            int64_t first_pts = 0;
            bool is_checked_first_pts = false;
            int64_t input_start_pts = 0;
            bool decode_ended = false;
            akashi::core::Rational cur_decode_pts = akashi::core::Rational(0, 1);
            AVMediaType media_type = AVMEDIA_TYPE_UNKNOWN;
        };

        struct InputSource {
            AVFormatContext* ifmt_ctx;

            core::VideoDecodeMethod decode_method = core::VideoDecodeMethod::NONE;

            size_t video_max_queue_count = 0;

            AVBufferRef* hw_device_ctx = nullptr;

            std::vector<DecodeStream> dec_streams;

            /* for decoding */

            uint8_t eof; // 1 if eof reached

            bool decode_started = false;

            bool decode_ended = false;

            /* from layer profile */

            const char* input_path;

            akashi::core::Rational from;

            akashi::core::Rational to;

            akashi::core::Rational start;

            // [TODO] if changed to std::string, liftime issues will occur
            const char* uuid;
        };

        void init_input_src(InputSource* input_src, const char* input_path);

        void free_input_src(InputSource*& input_src);

        int read_inputsrc(InputSource*& input_src, const char* input_path,
                          const core::VideoDecodeMethod& decode_method,
                          const size_t video_max_queue_count);

        int read_av_input(InputSource* input_src);

        int read_stream(InputSource* input_src);

    }
}
