#pragma once

#include <libakcore/hw_accel.h>

extern "C" {
#include <libavutil/avutil.h>
}

#include <string>

struct AVCodecContext;
struct AVCodec;
struct AVBufferRef;

namespace akashi {
    namespace codec {

        struct InputSource;
        struct FFFrameData;

        int create_hwdevice_ctx(AVBufferRef*& hw_device_ctx,
                                const core::VideoDecodeMethod decode_method,
                                const std::string& device_str);

        void show_hwdevice_info(AVBufferRef*& hw_device_ctx,
                                const core::VideoDecodeMethod decode_method,
                                bool show_profiles = false);

        // [XXX] For decoding only
        bool validate_hwconfig(core::VideoDecodeMethod* detected_method, const AVCodec* av_codec,
                               const core::VideoDecodeMethod pref_method);

        // [XXX] For decoding only
        enum AVPixelFormat configure_hwformat(AVCodecContext* cb_codec_ctx,
                                              const enum AVPixelFormat* pix_fmts);

        // [XXX] For decoding only
        void init_hwframe(FFFrameData& frame_data, const InputSource& input_src);
    }
}
