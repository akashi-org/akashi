#pragma once

#include "./encode_item.h"
#include <libakcore/memory.h>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace codec {

        struct EncodeArg;
        class FrameSink {
          public:
            explicit FrameSink(core::borrowed_ptr<state::AKState>){};
            virtual ~FrameSink(){};
            virtual EncodeResultCode send(const EncodeArg& encode_arg) = 0;
            virtual EncodeWriteResult write(const EncodeWriteArg& write_arg) = 0;
            virtual bool close(void) = 0;
        };

    }
}
