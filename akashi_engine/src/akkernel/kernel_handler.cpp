#include "./kernel_handler.h"
#include "./kernel_state.h"
#include "./kernel_event.h"
#include "./workers/transport.h"
#include "./workers/process.h"

#include <libakcore/logger.h>

#include <csignal>
#include <unistd.h>

namespace akashi {
    namespace kernel {

        namespace detail {

            // default case (pass-through)
            template <server::ASPMethod M = server::ASPMethod::INVALID>
            server::HTTPRPCResponse handle_asp_request_inner(
                HandlerContext& ctx,
                const KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED>& params) {
                if (!ctx.process_worker->get_thread_alive()) {
                    return server::error_rpc_res(params.request.id,
                                                 server::RPCErrorCode::INVALID_REQUEST,
                                                 "No renderer process found");
                }

                auto resp_str = ctx.process_worker->pass_through(params.req_str);
                auto rpc_res = server::parse_rpc_res(resp_str);
                if (std::holds_alternative<server::RPCResultObject>(rpc_res.payload)) {
                    auto payload = std::get<server::RPCResultObject>(rpc_res.payload);
                    return server::success_rpc_res(rpc_res.id, payload.value);
                } else {
                    auto payload = std::get<server::RPCErrorObject>(rpc_res.payload);
                    return server::error_rpc_res(rpc_res.id, payload.code, payload.data);
                }
            }

            template <>
            server::HTTPRPCResponse handle_asp_request_inner<server::ASPMethod::GENERAL_EVAL>(
                HandlerContext& ctx,
                const KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED>& params) {
                if (!ctx.process_worker->get_thread_alive()) {
                    ctx.process_worker->run({ctx.state, ctx.event_queue});
                    ctx.process_worker->wait_for_thread_alive();
                }
                // pass through
                return detail::handle_asp_request_inner(ctx, params);
            }

            template <>
            server::HTTPRPCResponse handle_asp_request_inner<server::ASPMethod::GENERAL_TERMINATE>(
                HandlerContext& ctx,
                const KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED>& params) {
                ctx.process_worker->terminate();

                return server::success_rpc_res(params.request.id, true);
            }

        }

        void handle_asp_request_received(HandlerContext& ctx) {
            auto params = std::get<KernelEventParams<KernelEvent::ASP_REQUEST_RECEIVED>>(
                ctx.event_data->params);

            server::HTTPRPCResponse http_res;
            if (params.request.method == server::ASPMethod::GENERAL_EVAL) {
                http_res =
                    detail::handle_asp_request_inner<server::ASPMethod::GENERAL_EVAL>(ctx, params);
            } else if (params.request.method == server::ASPMethod::GENERAL_TERMINATE) {
                http_res = detail::handle_asp_request_inner<server::ASPMethod::GENERAL_TERMINATE>(
                    ctx, params);
            } else {
                http_res = detail::handle_asp_request_inner(ctx, params);
            }

            ctx.transport_worker->set_asp_http_response(http_res);
            ctx.transport_worker->set_asp_handled(true);
        }

        void handle_process_exit(HandlerContext& ctx) {
            auto params =
                std::get<KernelEventParams<KernelEvent::PROCESS_EXIT>>(ctx.event_data->params);
            AKLOG_INFO("Kernel handled PROCESS_EXIT: {}", params.exit_code);
            // [XXX] exit_code may not be usable on Windows
            // https://www.boost.org/doc/libs/1_74_0/doc/html/boost_process/concepts.html#boost_process.concepts.process.exit_code
            // segv
            // if ((params.exit_code == SIGSEGV || params.exit_code == SIGABRT) &&
            //     ctx.state->prop().seq_process_error_exit < 3) {
            //     ctx.state->transform_prop(
            //         [](auto& new_prop) { new_prop.seq_process_error_exit += 1; });
            //     if (ctx.process_worker->get_thread_alive()) {
            //         ctx.process_worker->terminate();
            //     }
            //     ctx.process_worker->run({ctx.state, ctx.event_queue});
            // } else {
            //     ctx.state->transform_prop(
            //         [](auto& new_prop) { new_prop.seq_process_error_exit = 0; });
            // }
        }

    }
}
