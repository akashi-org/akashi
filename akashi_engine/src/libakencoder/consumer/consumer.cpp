#include "./consumer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        static void exec_encode(ConsumeLoopContext& ctx) {
            auto data = ctx.queue->dequeue();
            AKLOG_INFO("dequeued : {}", data.test_data);
            // exec encode
        }

        static void exit_thread(ConsumeLoopContext ctx) {
            ctx.state->set_consumer_finished(true, true);
        }

        void ConsumeLoop::consume_thread(ConsumeLoopContext ctx, ConsumeLoop* loop) {
            AKLOG_INFON("Consumer init");
            loop->set_on_thread_exit(
                [ctx](void*) {
                    exit_thread(ctx);
                    AKLOG_INFON("Consumer Successfully exited");
                },
                nullptr);

            while (!ctx.state->get_producer_finished()) {
                ctx.queue->wait_for_not_empty();
                exec_encode(ctx);
            }

            // draining
            while (ctx.queue->get_not_empty()) {
                exec_encode(ctx);
            }
            exit_thread(ctx);
            AKLOG_INFON("Consumer finished");
        }

    }
}
