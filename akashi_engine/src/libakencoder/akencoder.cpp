#include "./akencoder.h"
#include "./encode_queue.h"
#include "./producer/producer.h"
#include "./consumer/consumer.h"

#include <libakcore/logger.h>

#include <csignal>
#include <unistd.h>

// temporary
#include <chrono>
#include <thread>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ProduceLoop* produce_loop = nullptr;
            ConsumeLoop* consume_loop = nullptr;
        };

        void EncodeLoop::encode_thread(EncodeLoopContext ctx, EncodeLoop* loop) {
            AKLOG_INFON("Encoder init");

            EncodeQueue encode_queue{ctx.state};

            ProduceLoop produce_loop;
            ConsumeLoop consume_loop;

            ExitContext exit_ctx{&produce_loop, &consume_loop};

            loop->set_on_thread_exit(
                [](void* ctx) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx);
                    exit_ctx_->produce_loop->terminate();
                    exit_ctx_->consume_loop->terminate();
                    AKLOG_INFON("Successfully exited");
                },
                &exit_ctx);

            produce_loop.run({ctx.state, borrowed_ptr(&encode_queue)});
            consume_loop.run({ctx.state, borrowed_ptr(&encode_queue)});

            int wait_millsec = 5000;
            while (wait_millsec > 0) {
                AKLOG_INFON("...now encoding");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                wait_millsec -= 1000;
            }
            // send SIGTERM to the main thread
            kill(getpid(), SIGTERM);
        }

    }
}
