#include "./consumer.h"
#include "../encode_queue.h"

#include <libakstate/akstate.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        void ConsumeLoop::consume_thread(ConsumeLoopContext ctx, ConsumeLoop* loop) {
            AKLOG_INFON("Consumer init");
            loop->set_on_thread_exit([](void*) { AKLOG_INFON("Consumer Successfully exited"); },
                                     nullptr);

            while (true) {
                ctx.queue->wait_for_not_empty();
                auto data = ctx.queue->dequeue();
                AKLOG_INFO("dequeued : {}", data.test_data);
                // exec encode
            }
        }

    }
}
