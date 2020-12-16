#include "./input.h"

#include "./error.h"

#include <libakcore/logger.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include <vector>

namespace akashi {
    namespace codec {

        void init_input_src(InputSource* input_src, const char* input_path) {
            input_src->eof = 0;
            input_src->decode_started = false;
            input_src->decode_ended = false;
            input_src->input_path = input_path;
            input_src->ifmt_ctx = nullptr;
            input_src->uuid = "";
        }

        void free_input_src(InputSource*& input_src) {
            if (!input_src) {
                return;
            }

            if (input_src->ifmt_ctx != nullptr) {
                for (auto&& dec_stream : input_src->dec_streams) {
                    if (dec_stream.dec_ctx != nullptr) {
                        avcodec_free_context(&dec_stream.dec_ctx);
                        dec_stream.dec_ctx = nullptr;
                    }
                    if (dec_stream.swr_ctx != nullptr) {
                        // [TODO] need to flush the internal buffer before freeing?
                        swr_free(&dec_stream.swr_ctx);
                        dec_stream.swr_ctx = nullptr;
                    }
                }
                avformat_close_input(&input_src->ifmt_ctx);
                avformat_free_context(input_src->ifmt_ctx);
            }

            if (input_src != nullptr) {
                delete input_src;
                input_src = nullptr;
            }
        };

        int read_inputsrc(InputSource*& input_src, const char* input_path) {
            int ret_code = 0;

            input_src = new InputSource;
            init_input_src(input_src, input_path);
            // input_src[i].io_ctx = new IOContext(input_paths[i]);
            if (read_av_input(input_src) < 0) {
                ret_code = -1;
                goto exit;
            }
            if ((read_stream(input_src) < 0)) {
                ret_code = -1;
                goto exit;
            }
        exit:
            return ret_code;
        };

        int read_av_input(InputSource* input_src) {
            int ret = 0;

            ret = avformat_open_input(&(input_src->ifmt_ctx), input_src->input_path, nullptr,
                                      nullptr);

            if (ret < 0) {
                AKLOG_ERROR("avformat_open_input() failed, code={}({})", AVERROR(ret),
                            av_err2str(ret));
                goto end;
            }

            ret = avformat_find_stream_info(input_src->ifmt_ctx, nullptr);
            if (ret < 0) {
                AKLOG_ERROR("avformat_find_stream_info() failed, code={}({})", AVERROR(ret),
                            av_err2str(ret));
                goto end;
            }
            if (input_src->ifmt_ctx == nullptr) {
                ret = -1;
                AKLOG_ERRORN("failed to get avformat context");
                goto end;
            }

        end:

            return ret;
        };

        int read_stream(InputSource* input_src) {
            AVFormatContext* format_ctx = input_src->ifmt_ctx;
            input_src->dec_streams.reserve(format_ctx->nb_streams);
            input_src->dec_streams.resize(format_ctx->nb_streams);

            for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
                AVMediaType media_type = format_ctx->streams[i]->codecpar->codec_type;
                if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO ||
                    media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
                    AVCodecID codec_id = format_ctx->streams[i]->codecpar->codec_id;
                    AVCodec* av_codec = avcodec_find_decoder(codec_id);
                    if (av_codec == nullptr) {
                        AKLOG_ERROR("avcodec_find_decoder codec not found. codec_id={}", codec_id);
                        return -1;
                    }

                    input_src->dec_streams[i].dec_ctx = avcodec_alloc_context3(av_codec);
                    input_src->dec_streams[i].swr_ctx = nullptr;
                    input_src->dec_streams[i].swr_ctx_init_done = false;
                    input_src->dec_streams[i].is_checked_first_pts = false;
                    input_src->dec_streams[i].input_start_pts =
                        format_ctx->start_time == AV_NOPTS_VALUE ? 0 : format_ctx->start_time;
                    input_src->dec_streams[i].cur_decode_pts = akashi::core::Rational(0, 1);

                    AVCodecContext* codec_ctx = input_src->dec_streams[i].dec_ctx;

                    if (codec_ctx == nullptr) {
                        AKLOG_ERRORN("avcodec_alloc_context3() error");
                        return -1;
                    }

                    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[i]->codecpar) <
                        0) {
                        AKLOG_ERRORN("avcodec_parameters_to_context() error");
                        return -1;
                    }

                    int ret = 0;
                    ret = avcodec_open2(codec_ctx, av_codec, nullptr);
                    if (ret < 0) {
                        AKLOG_ERROR("avcodec_open2() failed, code={}", AVERROR(ret));
                        return -1;
                    }
                }
            }
            return 1;
        };

    }
}
