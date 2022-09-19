#include "./hwaccel.h"

#include "./source.h"
#include "./buffer.h"
#include "./utils.h"
#include "./error.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}

using namespace akashi::core;

namespace akashi {
    namespace codec {

        int create_hwdevice_ctx(AVBufferRef*& hw_device_ctx,
                                const core::VideoDecodeMethod decode_method,
                                const std::string& device_str) {
            switch (decode_method) {
                case core::VideoDecodeMethod::VAAPI:
                case core::VideoDecodeMethod::VAAPI_COPY: {
                    auto ret = av_hwdevice_ctx_create(
                        &hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI,
                        device_str.empty() ? nullptr : device_str.c_str(), nullptr, 0);
                    if (ret < 0) {
                        AKLOG_ERROR("av_hwdevice_ctx_create(() failed, code={}({})", AVERROR(ret),
                                    av_err2str(ret));
                        return ret;
                    }
                    break;
                }
                default: {
                }
            }

            return 0;
        }

        void show_hwdevice_info(AVBufferRef*& hw_device_ctx,
                                const core::VideoDecodeMethod decode_method, bool show_profiles) {
            switch (decode_method) {
                case core::VideoDecodeMethod::VAAPI:
                case core::VideoDecodeMethod::VAAPI_COPY: {
                    auto raw_hw_device_ctx = (AVHWDeviceContext*)hw_device_ctx->data;
                    auto dpy =
                        static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display;

                    AKLOG_INFO("VADisplay: {}", dpy ? "ok" : "null");
                    AKLOG_INFO("VA Vendor String: {}", vaQueryVendorString(dpy));

                    if (show_profiles) {
                        int va_max_num_profiles = vaMaxNumProfiles(dpy);
                        auto va_profiles = static_cast<VAProfile*>(
                            malloc(sizeof(VAProfile) * va_max_num_profiles));
                        if (va_profiles) {
                            int va_num_profiles = -1;
                            if (VAStatus status =
                                    vaQueryConfigProfiles(dpy, va_profiles, &va_num_profiles);
                                status != VA_STATUS_SUCCESS) {
                                AKLOG_INFO("vaQueryConfigProfiles() failed: {}",
                                           vaErrorStr(status));
                            } else {
                                for (int i = 0; i < va_num_profiles; i++) {
                                    AKLOG_INFO("VAProfiles: {}", static_cast<int>(va_profiles[i]));
                                }
                            }
                            free(va_profiles);
                        }
                    }
                    break;
                }
                default: {
                }
            }
        }

        bool validate_hwconfig(core::VideoDecodeMethod* detected_method, const AVCodec* av_codec,
                               const core::VideoDecodeMethod pref_method) {
            int config_idx = 0;
            auto hw_device_type = AV_HWDEVICE_TYPE_VAAPI;
            while (1) {
                const AVCodecHWConfig* config = avcodec_get_hw_config(av_codec, config_idx);
                if (!config) {
                    AKLOG_ERROR("Codec {} does not support hw device type {}", av_codec->name,
                                av_hwdevice_get_type_name(hw_device_type));
                    *detected_method = core::VideoDecodeMethod::SW;
                    return false;
                }
                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                    config->device_type == hw_device_type) {
                    // AKLOG_INFO("{}", av_get_pix_fmt_name(config->pix_fmt));
                    *detected_method = pref_method;
                    return true;
                }
                config_idx++;
            }

            return false;
        }

        enum AVPixelFormat configure_hwformat(AVCodecContext* cb_codec_ctx,
                                              const enum AVPixelFormat* pix_fmts) {
            auto input_src = reinterpret_cast<InputSource*>(cb_codec_ctx->opaque);

            enum AVPixelFormat last_sw_pix_fmt = AV_PIX_FMT_NONE;
            for (auto p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
                if (!is_hw_pix_fmt(*p)) {
                    last_sw_pix_fmt = *p;
                }
                if (*p == AV_PIX_FMT_VAAPI) {
                    if (input_src->decode_method == core::VideoDecodeMethod::VAAPI) {
                        cb_codec_ctx->hw_frames_ctx =
                            av_hwframe_ctx_alloc(input_src->hw_device_ctx);
                        auto hw_frames_ctx = (AVHWFramesContext*)cb_codec_ctx->hw_frames_ctx->data;

                        hw_frames_ctx->format = AV_PIX_FMT_VAAPI;
                        hw_frames_ctx->sw_format = cb_codec_ctx->sw_pix_fmt;
                        hw_frames_ctx->width = cb_codec_ctx->width;
                        hw_frames_ctx->height = cb_codec_ctx->height;

                        hw_frames_ctx->initial_pool_size = input_src->video_max_queue_count * 2;
                        hw_frames_ctx->pool = nullptr;

                        av_hwframe_ctx_init(cb_codec_ctx->hw_frames_ctx);
                    }

                    return *p;
                }
            }

            AKLOG_ERRORN("Failed to get a suitable HW surface format.");
            // try to fallback sw decoding
            input_src->decode_method = core::VideoDecodeMethod::SW;
            return last_sw_pix_fmt;
        }

        void init_hwframe(FFFrameData& frame_data, const InputSource& input_src) {
            if (input_src.decode_method == VideoDecodeMethod::VAAPI) {
                auto raw_hw_device_ctx = (AVHWDeviceContext*)input_src.hw_device_ctx->data;
                frame_data.va_display =
                    static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display;
            }
        }

    }
}
