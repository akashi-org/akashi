#include "./transport.h"
#include "../kernel_state.h"
#include "../kernel_event.h"

#include <libakserver/akserver.h>
#include <libakstate/akstate.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/config.h>

#include <csignal>
#include <unistd.h>

#include <chrono>
#include <thread>

using namespace akashi::core;
using namespace akashi::server;

namespace akashi {
    namespace kernel {

        struct TransportWorker::ExitContext {
            TransportWorkerContext ctx;
            TransportWorker* worker = nullptr;
            server::ServerHandle* server_handle = nullptr;
        };

        void TransportWorker::transport_thread(TransportWorkerContext ctx,
                                               TransportWorker* worker) {
            AKLOG_INFON("Transport init");

            ExitContext exit_ctx{ctx, worker};

            exit_ctx.server_handle = new server::ServerHandle{nullptr};

            worker->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    TransportWorker::exit_thread(*exit_ctx_);
                    AKLOG_INFON("Transport successfully exited");
                },
                &exit_ctx);

            init_kernel_asp_server(*exit_ctx.server_handle,
                                   (ASPConfig){"localhost", ctx.state->prop().port},
                                   [&ctx, worker](HTTPRPCResponse& res, const RPCRequest& req,
                                                  const std::string& req_str) {
                                       KernelEventQueueData evt_data;
                                       evt_data.evt = KernelEvent::ASP_REQUEST_RECEIVED;
                                       KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED> params;
                                       params.request = req;
                                       params.req_str = req_str;
                                       evt_data.params = params;

                                       worker->set_asp_handled(false);
                                       ctx.queue->enqueue(std::move(evt_data));
                                       worker->wait_for_asp_handled();

                                       res = worker->asp_http_response();
                                   });

            TransportWorker::exit_thread(exit_ctx);
            AKLOG_INFON("Transport finished");
        };

        void TransportWorker::exit_thread(ExitContext& exit_ctx) {
            if (exit_ctx.server_handle) {
                exit_ctx.server_handle->exit();
                delete exit_ctx.server_handle;
                exit_ctx.server_handle = nullptr;
            }

            if (exit_ctx.worker) {
                exit_ctx.worker->m_thread_exited = true;
            }
        }

    }
}
