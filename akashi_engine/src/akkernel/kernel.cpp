#include "./kernel.h"
#include "./kernel_state.h"
#include "./kernel_event.h"
#include "./kernel_handler.h"
#include "./workers/transport.h"
#include "./workers/process.h"

#include <libakstate/akstate.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/config.h>

#include <csignal>
#include <unistd.h>

#include <chrono>
#include <thread>

using namespace akashi::core;

namespace akashi {
    namespace kernel {

        struct KernelLoop::ExitContext {
            KernelLoopContext ctx;
            KernelLoop* loop = nullptr;
            TransportWorker* transport_worker = nullptr;
            ProcessWorker* process_worker = nullptr;
        };

        void KernelLoop::kernel_thread(KernelLoopContext ctx, KernelLoop* loop) {
            AKLOG_INFON("KernelLoop init");

            KernelEventQueue event_queue{};

            TransportWorker transport_worker;
            ProcessWorker process_worker;

            ExitContext exit_ctx{ctx, loop, &transport_worker, &process_worker};

            KernelState state;
            state.transform_prop([&ctx](auto& new_prop) {
                new_prop.config_jstr = ctx.config_jstr;
                new_prop.renderer_path = ctx.renderer_path;
                new_prop.port = ctx.port;
                new_prop.seq_process_error_exit = 0;
            });

            loop->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    KernelLoop::exit_thread(*exit_ctx_);
                    AKLOG_INFON("KernelLoop successfully exited");
                },
                &exit_ctx);

            transport_worker.run({core::borrowed_ptr(&state), core::borrowed_ptr(&event_queue)});

            while (true) {
                event_queue.wait_for_not_empty();
                auto evt_data = event_queue.dequeue();
                HandlerContext handler_ctx = {.transport_worker =
                                                  core::borrowed_ptr(&transport_worker),
                                              .process_worker = core::borrowed_ptr(&process_worker),
                                              .state = core::borrowed_ptr(&state),
                                              .event_queue = core::borrowed_ptr(&event_queue),
                                              .event_data = core::borrowed_ptr(&evt_data)};
                switch (evt_data.evt) {
                    case KernelEvent::ASP_REQUEST_RECEIVED: {
                        handle_asp_request_received(handler_ctx);
                        break;
                    }
                    case KernelEvent::PROCESS_EXIT: {
                        handle_process_exit(handler_ctx);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            KernelLoop::exit_thread(exit_ctx);
            AKLOG_INFON("KernelLoop finished");
        };

        void KernelLoop::exit_thread(ExitContext& exit_ctx) {
            if (exit_ctx.transport_worker) {
                exit_ctx.transport_worker->terminate();
            }

            if (exit_ctx.process_worker) {
                exit_ctx.process_worker->terminate();
            }

            if (exit_ctx.loop) {
                exit_ctx.loop->m_thread_exited = true;
            }

            // send SIGTERM to the main thread
            kill(getpid(), SIGTERM);
        }

    }
}
