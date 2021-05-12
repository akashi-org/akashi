#include "./server.h"
#include "./api.h"
#include "./json_rpc.h"

#include <libakcore/logger.h>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <exception>
#include <cassert>
#include <iostream>
#include <stdexcept>

#define RETURN_RESPONSE(res, rpc_res)                                                              \
    do {                                                                                           \
        res.status = rpc_res.status_code;                                                          \
        res.set_content(rpc_res.response_str, "application/json");                                 \
        return;                                                                                    \
    } while (0)

using namespace akashi::core;
using namespace httplib;

namespace akashi {

    namespace server {

        static void assert_api_set(const ASPAPISet& api_set) {
            assert(api_set.gui != nullptr);
            assert(api_set.media != nullptr);
            assert(api_set.general != nullptr);
        }

        static bool is_json_request(const Request& req) {
            if (!req.has_header("Content-type")) {
                return false;
            } else if (req.get_header_value("Content-type") != "application/json") {
                return false;
            } else {
                return true;
            }
        }

        static bool is_json_request(const std::string& req_str) {
            return nlohmann::json::accept(req_str);
        }

        struct NativeServerHandle {
            core::borrowed_ptr<httplib::Server> svr;
        };

        void ServerHandle::exit() {
            if (m_native_handle && m_native_handle->svr) {
                m_native_handle->svr->stop();
            } else {
                AKLOG_WARNN("ServerHandle is null");
            }
        }

        void ServerHandle::set_handle(NativeServerHandle* handle) noexcept(false) {
            if (m_native_handle) {
                throw std::runtime_error("ServerHandle already set");
            }
            m_native_handle = core::borrowed_ptr(handle);
        }

        void init_renderer_asp_server(const ASPAPISet&& api_set) {
            std::string input_str;

            assert_api_set(api_set);

            std::cout << "RENDERER_ASP_READY" << std::endl;

            while (true) {
                getline(std::cin, input_str);

                AKLOG_DEBUG("(kernel_asp@pipe) => {}", input_str);

                HTTPRPCResponse rpc_res;

                if (!is_json_request(input_str)) {
                    // usually not reachable
                    rpc_res = error_rpc_res("", RPCErrorCode::PARSE_ERROR, "Invalid json format");
                } else {
                    try {
                        rpc_res = exec_rpc(input_str, api_set);
                    } catch (const std::exception& e) {
                        rpc_res = error_rpc_res("", RPCErrorCode::SERVER_ERROR, e.what());
                    }
                }

                AKLOG_DEBUG("(renderer_asp@pipe) => {}", rpc_res.response_str.c_str());

                std::cout << rpc_res.response_str << std::endl;
            }
        }

        void init_kernel_asp_server(ServerHandle& handle, const ASPConfig& config,
                                    const OnRequest& on_request) {
            Server svr;

            svr.Post("/asp", [&on_request, config](const Request& req, Response& res) {
                HTTPRPCResponse rpc_res;
                RPCRequest rpc_req;

                AKLOG_INFO("(user@{}:{}) => {}", req.remote_addr.c_str(), req.remote_port,
                           req.body);

                if (!is_json_request(req)) {
                    rpc_res = error_rpc_res("", RPCErrorCode::PARSE_ERROR, "Invalid json format");
                    RETURN_RESPONSE(res, rpc_res);
                }

                try {
                    rpc_req = parse_rpc_req(req.body);
                } catch (const std::exception& e) {
                    rpc_res = error_rpc_res("", RPCErrorCode::SERVER_ERROR, e.what());
                    RETURN_RESPONSE(res, rpc_res);
                }

                on_request(rpc_res, rpc_req, req.body);

                AKLOG_INFO("(kernel_asp@{}:{}) => {}", config.host, config.port,
                           rpc_res.response_str.c_str());

                RETURN_RESPONSE(res, rpc_res);
            });

            AKLOG_INFO("...listening {}:{}", config.host, config.port);

            NativeServerHandle native_handle{core::borrowed_ptr(&svr)};
            handle.set_handle(&native_handle);

            svr.listen(config.host, config.port);
        }

    }
}
