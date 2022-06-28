#pragma once

#include <libakbuffer/avbuffer.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakcore/class.h>
#include <libakcore/hw_accel.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
}

#include <va/va.h>

struct AVPacket;

namespace akashi {
    namespace codec {

        struct DecodeStream;
        struct InputSource;

        struct FFAudioSpec {
            int sample_rate = 44100;
            int nb_samples = 0;
            AVSampleFormat format = AV_SAMPLE_FMT_FLT;
            int channels = 2;
            int channel_layout = AV_CH_LAYOUT_STEREO;
        };

        FFAudioSpec to_ff_audio_spec(const akashi::core::AKAudioSpec& spec, const int nb_samples);

        bool to_audio_payload(uint8_t*& out_buf, size_t* out_buf_size, FFAudioSpec& out_spec,
                              uint8_t* in_buf[AV_NUM_DATA_POINTERS], const FFAudioSpec& in_spec,
                              DecodeStream* dec_stream);

        struct FFFrameData {
            AVFrame* frame = nullptr;
            akashi::core::LayerProfile layer_prof;
            akashi::core::Rational pts;
            akashi::core::Rational rpts;
            akashi::core::AKAudioSpec out_audio_spec;
            buffer::AVBufferType media_type = buffer::AVBufferType::UNKNOWN;
            core::VideoDecodeMethod decode_method = core::VideoDecodeMethod::NONE;
            VADisplay va_display = nullptr;
        };

        class FFmpegBufferData final : public buffer::AVBufferData {
            AK_FORBID_COPY(FFmpegBufferData);

          public:
            explicit FFmpegBufferData(const FFFrameData& input, DecodeStream* dec_stream);
            virtual ~FFmpegBufferData();
            FFmpegBufferData(FFmpegBufferData&& buf_data) = default;

          private:
            void populate_video(const FFFrameData& input);
            void populate_audio(const FFFrameData& input, DecodeStream* dec_stream);

          private:
            int m_linesize[AV_NUM_DATA_POINTERS];
            AVFrame* m_frame = nullptr;
        };

    }
}
