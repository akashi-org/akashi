#pragma once

#include "./encode_item.h"

#include <libakcore/memory.h>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace codec {

        class FrameSink;
        class AKEncoder final {
          public:
            explicit AKEncoder(core::borrowed_ptr<state::AKState> state);
            virtual ~AKEncoder();

            EncodeResultCode send(const EncodeArg& encode_arg);

            EncodeWriteResult write(const EncodeWriteArg& write_arg);

            bool close(void);

          private:
            core::owned_ptr<FrameSink> m_frame_sink;
        };

    }
}
