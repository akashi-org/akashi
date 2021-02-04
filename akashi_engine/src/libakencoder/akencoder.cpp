#include "./akencoder.h"
#include "./encode_queue.h"
#include "./encode_state.h"
#include "./producer/producer.h"
#include "./consumer/consumer.h"

#include <libakcore/logger.h>

#include <csignal>
#include <unistd.h>

// temporary
#include <chrono>
#include <thread>

#include <mutex>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ProduceLoop* produce_loop = nullptr;
            ConsumeLoop* consume_loop = nullptr;
        };

        static void exit_thread(const ExitContext& exit_ctx) {
            exit_ctx.produce_loop->terminate();
            exit_ctx.consume_loop->terminate();

            // send SIGTERM to the main thread
            kill(getpid(), SIGTERM);
        }

        void EncodeLoop::encode_thread(EncodeLoopContext ctx, EncodeLoop* loop) {
            AKLOG_INFON("Encoder init");

            EncodeQueue encode_queue{ctx.state};

            ProduceLoop produce_loop;
            ConsumeLoop consume_loop;

            ExitContext exit_ctx{&produce_loop, &consume_loop};

            loop->set_on_thread_exit(
                [](void* ctx) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx);
                    AKLOG_INFON("Successfully exited");
                    exit_thread(*exit_ctx_);
                },
                &exit_ctx);

            produce_loop.run({ctx.state, borrowed_ptr(&encode_queue)});
            consume_loop.run({ctx.state, borrowed_ptr(&encode_queue)});

            ctx.state->wait_for_producer_finished();
            ctx.state->wait_for_consumer_finished();

            AKLOG_INFON("Encoder finished");
            exit_thread(exit_ctx);
        }

    }
}
