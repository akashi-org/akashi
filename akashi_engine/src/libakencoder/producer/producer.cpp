#include "./producer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>

// temporary
#include <chrono>
#include <thread>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        static void exit_thread(ProduceLoopContext ctx) {
            ctx.state->set_producer_finished(true, true);
        }

        void ProduceLoop::produce_thread(ProduceLoopContext ctx, ProduceLoop* loop) {
            AKLOG_INFON("Producer init");
            loop->set_on_thread_exit(
                [ctx](void*) {
                    exit_thread(ctx);
                    AKLOG_INFON("Producer Successfully exited");
                },
                nullptr);

            int cnt = 0;
            // enqueue data until all frames processed
            while (cnt < 100) {
                ctx.queue->wait_for_not_full();
                // eval
                // decode
                // render
                // enqueue

                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                ctx.queue->enqueue({cnt});
                cnt += 1;
            }
            exit_thread(ctx);
            AKLOG_INFON("Producer finished");
        }

    }
}
