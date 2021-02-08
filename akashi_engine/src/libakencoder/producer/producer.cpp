#include "./producer.h"
#include "../encode_queue.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/path.h>
#include <libakstate/akstate.h>
#include <libakeval/akeval.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct ExitContext {
            ProduceLoopContext ctx;
            eval::AKEval* eval = nullptr;
        };
        struct EncodeContext {
            Rational cur_pts;
            Rational fps;
            Rational duration;
            core::Path entry_path{""};
        };

        static void exit_thread(ExitContext& exit_ctx) {
            if (exit_ctx.eval) {
                exit_ctx.eval->exit();
            }
            exit_ctx.ctx.state->set_producer_finished(true, true);
        }

        static EncodeContext create_encode_context(ProduceLoopContext& ctx,
                                                   core::borrowed_ptr<eval::AKEval> eval) {
            Rational fps;
            core::Path entry_path{""};
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
                fps = ctx.state->m_prop.fps;
            }

            auto profile = eval->render_prof(entry_path.to_abspath().to_str());
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                ctx.state->m_prop.render_prof = profile;
            }

            return {.cur_pts = Rational(0, 1),
                    .fps = fps,
                    .duration = to_rational(profile.duration),
                    .entry_path = entry_path};
        }

        static void update_encode_context(EncodeContext& encode_ctx) {
            encode_ctx.cur_pts += (Rational(1, 1) / encode_ctx.fps);
        }

        static bool can_produce(const EncodeContext& encode_ctx) {
            // [TODO] exclusive?
            return encode_ctx.cur_pts <= encode_ctx.duration;
        }

        static std::vector<core::FrameContext>
        pull_frame_context(core::borrowed_ptr<eval::AKEval> eval, const EncodeContext& encode_ctx) {
            return eval->eval_krons(encode_ctx.entry_path.to_abspath().to_str(), encode_ctx.cur_pts,
                                    encode_ctx.fps.to_decimal(), encode_ctx.duration, 1);
        }

        void ProduceLoop::produce_thread(ProduceLoopContext ctx, ProduceLoop* loop) {
            AKLOG_INFON("Producer init");

            auto eval = make_owned<eval::AKEval>(borrowed_ptr(ctx.state));

            ExitContext exit_ctx{ctx, eval.get()};

            loop->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    exit_thread(*exit_ctx_);
                    AKLOG_INFON("Producer Successfully exited");
                },
                &exit_ctx);

            // enqueue data until all frames processed
            for (
                /* clang-format off */
                auto encode_ctx = create_encode_context(ctx, borrowed_ptr(eval)); 
                can_produce(encode_ctx);
                update_encode_context(encode_ctx)
                /* clang-format on */
            ) {
                ctx.queue->wait_for_not_full();

                // eval
                auto frame_ctx = pull_frame_context(borrowed_ptr(eval), encode_ctx);

                // decode
                // render
                // enqueue

                ctx.queue->enqueue({encode_ctx.cur_pts.to_decimal()});
            }

            exit_thread(exit_ctx);
            AKLOG_INFON("Producer finished");
        }

    }
}
