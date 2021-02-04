#include "./producer.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        void ProduceLoop::produce_thread(ProduceLoopContext, ProduceLoop* loop) {
            AKLOG_INFON("Producer init");
            loop->set_on_thread_exit([](void*) { AKLOG_INFON("Producer Successfully exited"); },
                                     nullptr);
        }

    }
}
