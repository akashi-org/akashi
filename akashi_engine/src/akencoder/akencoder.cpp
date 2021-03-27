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

        static void early_exit() { kill(getpid(), SIGTERM); }

        void EncodeLoop::encode_thread(EncodeLoopContext ctx, EncodeLoop* loop) {
            AKLOG_INFON("Encoder init");

            codec::AKEncoder encoder{ctx.state};

            if (ctx.state->m_encode_conf.audio_codec != core::EncodeCodec::NONE) {
                // [XXX] must be done before decoder initialization
                auto aformat = encoder.validate_audio_format(
                    ctx.state->m_atomic_state.encode_audio_spec.load().format);
                if (aformat == AKAudioSampleFormat::NONE) {
                    return early_exit();
                } else {
                    auto decode_spec = ctx.state->m_atomic_state.audio_spec.load();
                    decode_spec.format = aformat;
                    ctx.state->m_atomic_state.audio_spec.store(decode_spec);
                    ctx.state->m_atomic_state.encode_audio_spec.store(decode_spec);
                }
            }
            if (!encoder.open()) {
                return early_exit();
            }

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
