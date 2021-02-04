#include "./consumer.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        void ConsumeLoop::consume_thread(ConsumeLoopContext, ConsumeLoop* loop) {
            AKLOG_INFON("Consumer init");
            loop->set_on_thread_exit([](void*) { AKLOG_INFON("Consumer Successfully exited"); },
                                     nullptr);
        }

    }
}
