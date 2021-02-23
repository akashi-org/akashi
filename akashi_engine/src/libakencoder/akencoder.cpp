#include "./akencoder.h"
#include "./encode_queue.h"
#include "./producer/producer.h"
#include "./consumer/consumer.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakcodec/encoder.h>

#include <csignal>
#include <unistd.h>

#include <mutex>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ProduceLoop* produce_loop;
            ConsumeLoop* consume_loop;
            EncodeLoop* loop;
        };

        void EncodeLoop::encode_thread(EncodeLoopContext ctx, EncodeLoop* loop) {
            AKLOG_INFON("Encoder init");

            codec::AKEncoder encoder{ctx.state};
            EncodeQueue encode_queue{ctx.state};

            ProduceLoop produce_loop;
            ConsumeLoop consume_loop;

            ExitContext exit_ctx{&produce_loop, &consume_loop, loop};

            loop->set_on_thread_exit(
                [](void* ctx) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx);
                    EncodeLoop::exit_thread(*exit_ctx_);
                    AKLOG_INFON("Encoder Successfully exited");
                },
                &exit_ctx);

            produce_loop.run({ctx.state, borrowed_ptr(&encoder), borrowed_ptr(&encode_queue)});
            consume_loop.run({ctx.state, borrowed_ptr(&encoder), borrowed_ptr(&encode_queue)});

            ctx.state->wait_for_producer_finished();
            ctx.state->wait_for_consumer_finished();

            AKLOG_INFON("Encoder finished");
            EncodeLoop::exit_thread(exit_ctx);
        }

        void EncodeLoop::exit_thread(ExitContext& exit_ctx) {
            exit_ctx.produce_loop->terminate();
            exit_ctx.consume_loop->terminate();

            exit_ctx.loop->m_thread_exited = true;

            // send SIGTERM to the main thread
            kill(getpid(), SIGTERM);
        }

    }
}
