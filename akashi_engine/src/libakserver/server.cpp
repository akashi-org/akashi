#include "./server.h"
#include "./api.h"
#include "./json_rpc.h"

#include <libakcore/logger.h>

#include <httplib.h>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <string_view>
#include <exception>
#include <cassert>

#include <iostream>

#define RETURN_RESPONSE(res, rpc_res)                                                              \
    do {                                                                                           \
        res.status = rpc_res.status_code;                                                          \
        res.set_content(rpc_res.result, "application/json");                                       \
        return;                                                                                    \
    } while (0)

using namespace akashi::core;
using namespace httplib;
using namespace std::chrono;

namespace akashi {

    namespace server {

        static const std::string get_system_clock() {
            auto now_t = system_clock::to_time_t(system_clock::now());
            char mbstr[100];
            std::strftime(mbstr, sizeof(mbstr), "%c", std::localtime(&now_t));
            return std::string(mbstr);
        }

        static void log_recv(const Request& req, const Response&) {
            // clang-format off
            AKLOG_INFO("[{}] {} => {} {} {}", 
                get_system_clock().c_str(), 
                req.remote_addr.c_str(), req.method.c_str(), req.path.c_str(), req.version.c_str());
            // clang-format on
        }

        static void log_send(const Request&, const Response& res) {
            // clang-format off
            AKLOG_INFO("[{}] self => {} {}", 
                get_system_clock().c_str(),
                res.status, res.version.c_str());
            // clang-format on
        }

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

        void init_asp_server(const ASPConfig& config, const ASPAPISet&& api_set) {
            // std::string input_str;

            // while (true) {
            //     getline(std::cin, input_str);
            //     RPCResult rpc_res;

            //     // if (!is_json_request(req)) {
            //     //      rpc_res = error_rpc_res("", RPCErrorCode::PARSE_ERROR, "Invalid json
            //     //      format"); RETURN_RESPONSE(res, rpc_res);
            //     // }

            //     try {
            //         rpc_res = exec_rpc(input_str, api_set);
            //     } catch (const std::exception& e) {
            //         rpc_res = error_rpc_res("", RPCErrorCode::SERVER_ERROR, e.what());
            //     }

            // }

            Server svr;

            assert_api_set(api_set);

            svr.Post("/asp", [&api_set](const Request& req, Response& res) {
                log_recv(req, res);

                RPCResult rpc_res;

                if (!is_json_request(req)) {
                    rpc_res = error_rpc_res("", RPCErrorCode::PARSE_ERROR, "Invalid json format");
                    RETURN_RESPONSE(res, rpc_res);
                }

                try {
                    rpc_res = exec_rpc(req.body, api_set);
                } catch (const std::exception& e) {
                    rpc_res = error_rpc_res("", RPCErrorCode::SERVER_ERROR, e.what());
                    RETURN_RESPONSE(res, rpc_res);
                }

                RETURN_RESPONSE(res, rpc_res);
            });

            svr.set_logger([](const Request& req, const Response& res) { log_send(req, res); });

            AKLOG_INFO("...listening {}:{}", config.host, config.port);

            svr.listen(config.host, config.port);
        }

    }
}
