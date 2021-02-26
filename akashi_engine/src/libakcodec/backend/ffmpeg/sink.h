#pragma once

#include "../../sink.h"
#include "../../encode_item.h"

#include <libakcore/memory.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVBufferRef;
struct SwsContext;

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace codec {

        struct EncodeStream {
            AVCodecContext* enc_ctx = nullptr;
            AVStream* enc_stream = nullptr;
            struct SwsContext* sws_ctx = nullptr;
        };

        struct EncodeArg;
        class FFFrameSink : public FrameSink {
          public:
            explicit FFFrameSink(core::borrowed_ptr<state::AKState> state);
            virtual ~FFFrameSink();

            virtual EncodeResultCode send(const EncodeArg& encode_arg) override;

            virtual EncodeWriteResult write(const EncodeWriteArg& write_arg) override;

            virtual size_t nb_samples_per_frame(void) override;

            virtual bool close(void) override;

          private:
            bool init_video_stream();

            bool init_audio_stream();

            bool init_video_frame(AVFrame** frame, const EncodeArg& encode_arg);

            bool populate_video_frame(AVFrame* frame, const EncodeArg& encode_arg);

            void flush_encoder(const buffer::AVBufferType& type);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            AVFormatContext* m_ofmt_ctx = nullptr;

            EncodeStream m_video_stream;
            EncodeStream m_audio_stream;

            bool m_encoder_flushed = false;
        };
    }
}
