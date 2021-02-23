#include "./consumer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakcodec/encoder.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ConsumeLoopContext ctx;
            ConsumeLoop* loop;
        };

        static void exec_encode(ConsumeLoopContext& ctx) {
            // send until EAGAIN or ERROR
            while (true) {
                const auto& data = ctx.queue->top();
                if (!data.buffer) {
                    break;
                }
                auto send_result = ctx.encoder->send(data);
                if (send_result == codec::EncodeResultCode::OK) {
                    ctx.queue->dequeue();
                } else if (send_result == codec::EncodeResultCode::SEND_EAGAIN) {
                    // AKLOG_DEBUG("SEND_EAGAIN {}", data.pts.to_decimal());
                    break;
                } else {
                    // [TODO] what should we do here?
                    AKLOG_ERROR("encode error {}, {}", data.pts.to_decimal(), send_result);
                    return;
                }
            }

            // recv and write until EAGAIN or ERROR
            while (true) {
                auto write_result = ctx.encoder->write({buffer::AVBufferType::VIDEO});
                switch (write_result.result) {
                    case codec::EncodeResultCode::ERROR: {
                        AKLOG_ERRORN("Encode error while writing");
                        return;
                    }
                    case codec::EncodeResultCode::RECV_EAGAIN: {
                        // AKLOG_DEBUGN("RECV_EAGAIN");
                        return;
                    }
                    default: {
                    }
                }
            }
        }

        void ConsumeLoop::consume_thread(ConsumeLoopContext ctx, ConsumeLoop* loop) {
            AKLOG_INFON("Consumer init");

            ExitContext exit_ctx{ctx, loop};

            loop->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    ConsumeLoop::exit_thread(*exit_ctx_);
                    AKLOG_INFON("Consumer Successfully exited");
                },
                &exit_ctx);

            while (!ctx.state->get_producer_finished()) {
                ctx.queue->wait_for_not_empty();
                exec_encode(ctx);
            }

            // draining
            while (ctx.queue->get_not_empty()) {
                exec_encode(ctx);
            }
            ctx.encoder->close();

            ConsumeLoop::exit_thread(exit_ctx);
            AKLOG_INFON("Consumer finished");
        }

        void ConsumeLoop::exit_thread(ExitContext& exit_ctx) {
            exit_ctx.loop->m_thread_exited = true;
            exit_ctx.ctx.state->set_consumer_finished(true, true);
        }
    }
}
