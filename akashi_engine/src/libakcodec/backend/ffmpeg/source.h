#pragma once

#include "../../source.h"
#include "../../decode_item.h"

#include <libakcore/element.h>
#include <libakcore/class.h>

struct AVPacket;
struct AVFrame;
struct AVCodecContext;

namespace akashi {
    namespace buffer {
        class AVBufferData;
    }
    namespace core {
        class Rational;
        enum class VideoDecodeMethod;
    }
    namespace codec {

        struct InputSource;
        struct DecodeResult;
        struct DecodeArg;

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

            virtual bool done_init(void) const override { return m_done_init; }

            core::Rational dts(void) const override;

            const core::LayerProfile& layer_profile() const override;

          private:
            int decode_packet(AVPacket* pkt, AVFrame* frame, AVCodecContext* dec_ctx);

            bool is_streams_end(void) const;

          private:
            InputSource* m_input_src = nullptr;
            AVPacket* m_pkt = nullptr;
            AVFrame* m_proxy_frame = nullptr;
            AVFrame* m_frame = nullptr;
            bool m_done_init = false;
        };
    }
}
