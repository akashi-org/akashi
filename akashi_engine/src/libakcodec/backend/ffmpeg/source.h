#pragma once

#include "../../source.h"
#include "../../decode_item.h"

#include <libakcore/class.h>

struct AVPacket;
struct AVFrame;

namespace akashi {
    namespace buffer {
        class AVBufferData;
    }
    namespace core {
        struct LayerProfile;
        class Rational;
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
                              const core::Rational& decode_start) override;

            virtual DecodeResult decode(const DecodeArg& decode_arg) override;

            bool seek(const core::Rational& seek_pts);

            virtual void finalize(void) override;

            virtual bool can_decode(void) const override;

            virtual bool done_init(void) const override { return m_done_init; }

          private:
            bool is_streams_end(void) const;

          private:
            InputSource* m_input_src = nullptr;
            AVPacket* m_pkt = nullptr;
            AVFrame* m_frame = nullptr;
            bool m_done_init = false;
        };
    }
}
