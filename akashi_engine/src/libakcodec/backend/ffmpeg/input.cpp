#include "./input.h"

#include "./error.h"

#include <libakcore/logger.h>
#include <libakcore/hw_accel.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
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

            if (input_src->hw_device_ctx != nullptr) {
                av_buffer_unref(&input_src->hw_device_ctx);
                input_src->hw_device_ctx = nullptr;
            }

            if (input_src != nullptr) {
                delete input_src;
                input_src = nullptr;
            }
        };

        int read_inputsrc(InputSource*& input_src, const char* input_path,
                          const core::VideoDecodeMethod& decode_method,
                          const size_t video_max_queue_count) {
            int ret_code = 0;

            input_src = new InputSource;
            init_input_src(input_src, input_path);

            input_src->decode_method = decode_method;
            input_src->video_max_queue_count = video_max_queue_count;

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

            switch (input_src->decode_method) {
                case core::VideoDecodeMethod::VAAPI:
                case core::VideoDecodeMethod::VAAPI_COPY: {
                    ret = av_hwdevice_ctx_create(&input_src->hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI,
                                                 NULL, NULL, 0);
                    if (ret < 0) {
                        AKLOG_ERROR("av_hwdevice_ctx_create(() failed, code={}({})", AVERROR(ret),
                                    av_err2str(ret));
                        goto end;
                    }

                    {
                        auto raw_hw_device_ctx = (AVHWDeviceContext*)input_src->hw_device_ctx->data;
                        auto dpy =
                            static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display;
                        AKLOG_INFO("VADisplay: {}", dpy ? "ok" : "null");
                        AKLOG_INFO("VA Vendor String: {}", vaQueryVendorString(dpy));

                        // int va_max_num_profiles = vaMaxNumProfiles(dpy);
                        // auto va_profiles = static_cast<VAProfile*>(
                        //     malloc(sizeof(VAProfile) * va_max_num_profiles));
                        // if (va_profiles) {
                        //     int va_num_profiles = -1;
                        //     if (VAStatus status =
                        //             vaQueryConfigProfiles(dpy, va_profiles, &va_num_profiles);
                        //         status != VA_STATUS_SUCCESS) {
                        //         AKLOG_INFO("vaQueryConfigProfiles() failed: {}",
                        //                    vaErrorStr(status));
                        //     } else {
                        //         for (int i = 0; i < va_num_profiles; i++) {
                        //             AKLOG_INFO("VAProfiles: {}",
                        //             static_cast<int>(va_profiles[i]));
                        //         }
                        //     }
                        //     free(va_profiles);
                        // }
                    }
                    break;
                }
                default: {
                }
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
                input_src->dec_streams[i].media_type = media_type;
                if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO ||
                    media_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
                    AVCodecID codec_id = format_ctx->streams[i]->codecpar->codec_id;
                    AVCodec* av_codec = avcodec_find_decoder(codec_id);
                    if (av_codec == nullptr) {
                        AKLOG_ERROR("avcodec_find_decoder codec not found. codec_id={}", codec_id);
                        return -1;
                    }

                    input_src->dec_streams[i].is_active = true;
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

                    if (media_type == AVMediaType::AVMEDIA_TYPE_VIDEO && input_src->hw_device_ctx) {
                        codec_ctx->hw_device_ctx = av_buffer_ref(input_src->hw_device_ctx);
                        if (!codec_ctx->hw_device_ctx) {
                            AKLOG_ERRORN("A hardware device reference create failed");
                            return -1;
                        }
                        codec_ctx->get_format = [](AVCodecContext* cb_codec_ctx,
                                                   const enum AVPixelFormat* pix_fmts) {
                            const enum AVPixelFormat* p;

                            for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
                                if (*p == AV_PIX_FMT_VAAPI) {
                                    auto input_src =
                                        reinterpret_cast<InputSource*>(cb_codec_ctx->opaque);
                                    if (input_src->decode_method ==
                                        core::VideoDecodeMethod::VAAPI) {
                                        cb_codec_ctx->hw_frames_ctx =
                                            av_hwframe_ctx_alloc(input_src->hw_device_ctx);
                                        auto hw_frames_ctx =
                                            (AVHWFramesContext*)cb_codec_ctx->hw_frames_ctx->data;

                                        hw_frames_ctx->format = AV_PIX_FMT_VAAPI;
                                        hw_frames_ctx->sw_format = cb_codec_ctx->sw_pix_fmt;
                                        hw_frames_ctx->width = cb_codec_ctx->width;
                                        hw_frames_ctx->height = cb_codec_ctx->height;

                                        hw_frames_ctx->initial_pool_size =
                                            input_src->video_max_queue_count * 2;
                                        hw_frames_ctx->pool = nullptr;

                                        av_hwframe_ctx_init(cb_codec_ctx->hw_frames_ctx);
                                    }

                                    return *p;
                                }
                            }

                            AKLOG_ERRORN("Failed to get a suitable HW surface format. ");
                            return AV_PIX_FMT_NONE;
                        };

                        codec_ctx->opaque = input_src;
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
