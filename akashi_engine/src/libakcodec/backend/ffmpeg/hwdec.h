#pragma once

#include <libakcore/hw_accel.h>

extern "C" {
#include <libavutil/avutil.h>
}

struct AVCodecContext;
struct AVCodec;

namespace akashi {
    namespace codec {

        struct InputSource;
        struct FFFrameData;

        int create_hwdevice_ctx(InputSource& input_src,
                                const core::VideoDecodeMethod decode_method);

        void show_hwdevice_info(const InputSource& input_src,
                                const core::VideoDecodeMethod decode_method);

        bool validate_hwconfig(core::VideoDecodeMethod* detected_method, const AVCodec* av_codec,
                               const core::VideoDecodeMethod pref_method);

        enum AVPixelFormat configure_hwformat(AVCodecContext* cb_codec_ctx,
                                              const enum AVPixelFormat* pix_fmts);

        void init_hwframe(FFFrameData& frame_data, const InputSource& input_src);
    }
}
