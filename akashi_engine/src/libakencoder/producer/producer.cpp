#include "./producer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        void ProduceLoop::produce_thread(ProduceLoopContext ctx, ProduceLoop* loop) {
            AKLOG_INFON("Producer init");
            loop->set_on_thread_exit([](void*) { AKLOG_INFON("Producer Successfully exited"); },
                                     nullptr);

            bool finished = false;
            int cnt = 0;
            // enqueue data until all frames processed
            while (!finished) {
                ctx.queue->wait_for_not_full();
                // eval
                // decode
                // render
                // enqueue

                ctx.queue->enqueue({cnt});
                cnt += 1;
            }
        }

    }
}
