#pragma once

#include <libakcore/rational.h>
#include <libakcore/element.h>
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
            bool is_active = false; // if false, we do not use this stream
            AVCodecContext* dec_ctx = nullptr;
            struct SwrContext* swr_ctx = nullptr;
            bool swr_ctx_init_done = false;
            int64_t first_pts = 0;
            bool is_checked_first_pts = false;
            int64_t input_start_pts = 0;
            bool decode_ended = false;
            akashi::core::Rational cur_decode_pts = akashi::core::Rational(0, 1);
            AVMediaType media_type = AVMEDIA_TYPE_UNKNOWN;
            int64_t conv_effective_pts = 0;
        };

        struct InputSource {
            AVFormatContext* ifmt_ctx;

            core::VideoDecodeMethod decode_method = core::VideoDecodeMethod::NONE;

            core::VideoDecodeMethod preferred_decode_method = core::VideoDecodeMethod::NONE;

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

            size_t loop_cnt = 0;

            akashi::core::Rational act_dur = akashi::core::Rational(0, 1);

            // [TODO] if changed to std::string, liftime issues will occur
            const char* uuid;

            // [TODO] need to merge
            core::LayerProfile layer_profile;
        };

        void init_input_src(InputSource* input_src, const char* input_path);

        void free_input_src(InputSource*& input_src);

        int read_inputsrc(InputSource*& input_src, const core::LayerProfile& layer_profile,
                          const core::VideoDecodeMethod& preferred_decode_method,
                          const size_t video_max_queue_count);

        int read_av_input(InputSource* input_src);

        int read_stream(InputSource* input_src);

    }
}
