#pragma once

#include "../../source.h"
#include "../../decode_item.h"

#include <libakcore/element.h>
#include <libakcore/class.h>
#include <libakcore/hw_accel.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

struct AVPacket;
struct AVFrame;
struct AVCodecContext;
struct AVFormatContext;

namespace akashi {
    namespace buffer {
        class AVBufferData;
    }
    namespace core {
        class Rational;
        enum class VideoDecodeMethod;
    }
    namespace codec {

        struct DecodeStream {
            bool is_active = false; // if false, we do not use this stream
            bool decode_ended = false;

            AVMediaType media_type = AVMEDIA_TYPE_UNKNOWN;

            AVCodecContext* dec_ctx = nullptr;

            struct SwrContext* swr_ctx = nullptr;
            bool swr_ctx_init_done = false;

            int64_t first_pts = 0;
            bool is_checked_first_pts = false;
            int64_t input_start_pts = 0;

            akashi::core::Rational cur_decode_pts = akashi::core::Rational(0, 1);
            int64_t conv_effective_pts = 0;
        };

        struct InputSource {
            AVFormatContext* ifmt_ctx = nullptr;
            AVBufferRef* hw_device_ctx = nullptr;
            AVPacket* pkt = nullptr;
            AVFrame* proxy_frame = nullptr;
            AVFrame* frame = nullptr;

            std::vector<DecodeStream> dec_streams;

            core::VideoDecodeMethod decode_method = core::VideoDecodeMethod::NONE;
            core::VideoDecodeMethod preferred_decode_method = core::VideoDecodeMethod::NONE;

            size_t video_max_queue_count = 0;
            bool init_called = false;
            uint8_t eof = 0; // 1 if eof reached
            bool decode_ended = false;
            size_t loop_cnt = 0;
            akashi::core::Rational act_dur = akashi::core::Rational(0, 1);

            core::LayerProfile layer_prof;
        };

        struct DecodeResult;
        struct DecodeArg;
        class PTSSet;

        class FFLayerSource : public LayerSource {
            AK_FORBID_COPY(FFLayerSource);

          public:
            explicit FFLayerSource() = default;
            virtual ~FFLayerSource();

            virtual bool init(const core::LayerProfile& layer_profile,
                              const core::Rational& decode_start,
                              const core::VideoDecodeMethod& preferred_decode_method,
                              const size_t video_max_queue_count) override;

            virtual DecodeResult decode(const DecodeArg& decode_arg) override;

            bool seek(const core::Rational& seek_pts);

            virtual void finalize(void) override;

            virtual bool can_decode(void) const override;

            virtual bool done_init(void) const override;

            core::Rational dts(void) const override;

            const core::LayerProfile& layer_profile() const override;

          private:
            bool init_inputsrc(const core::LayerProfile& layer_profile,
                               const core::Rational& decode_start,
                               const core::VideoDecodeMethod& preferred_decode_method,
                               const size_t video_max_queue_count);

            int read_avformat();

            int read_avstream();

            bool demux_priv(DecodeResult* decode_result);

            int decode_packet(AVPacket* pkt, AVFrame* frame, AVCodecContext* dec_ctx);

            bool decode_priv(DecodeResult* decode_result);

            bool validate_pts(DecodeResult* decode_result, const PTSSet& pts_set);

            void populate_buffer(DecodeResult* decode_result, const DecodeArg& decode_arg,
                                 const PTSSet& pts_set);

            bool all_streams_ended(void) const;

          private:
            InputSource m_input_src;
        };
    }
}
