#include "./json_rpc.h"
#include "./api.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

#define PARSE_REQUEST(v, k, v_type)                                                                \
    try {                                                                                          \
        v = req_j.at(k).get<v_type>();                                                             \
    } catch (const std::exception& e) {                                                            \
        return error_rpc_res("", RPCErrorCode::INVALID_REQUEST, e.what());                         \
    }

#define PARSE_PARAMS(req_j, res_j, params, ...)                                                    \
    std::tuple<__VA_ARGS__> params;                                                                \
    try {                                                                                          \
        parse_rpc_request_params((req_j), (params));                                               \
    } catch (const std::exception& e) {                                                            \
        return error_rpc_res((res_j)["id"], RPCErrorCode::INVALID_PARAMS, e.what());               \
    }

#define EXEC_METHOD(res_j, api_set, method, params)                                                \
    try {                                                                                          \
        (res_j)["result"] =                                                                        \
            std::apply([&api_set](auto&... args) { return method(args...); }, (params));           \
    } catch (const std::exception& e) {                                                            \
        return error_rpc_res((res_j)["id"], RPCErrorCode::INTERNAL_ERROR, e.what());               \
    }

#define EXEC_METHOD_NO_PARAMS(res_j, api_set, method)                                              \
    try {                                                                                          \
        (res_j)["result"] = method();                                                              \
    } catch (const std::exception& e) {                                                            \
        return error_rpc_res((res_j)["id"], RPCErrorCode::INTERNAL_ERROR, e.what());               \
    }

namespace akashi {
    namespace server {

        template <typename T>
        void parse_rpc_request_params(const nlohmann::json& j, T& params) noexcept(false) {
            params = j.at("params").get<T>();
        }

        RPCResult error_rpc_res(const std::string& id, const RPCErrorCode& e_code,
                                const std::string& e_msg) {
            // @ref: https://www.jsonrpc.org/specification
            // @ref: https://www.jsonrpc.org/historical/json-rpc-over-http.html

            RPCResult rpc_res;

            nlohmann::json j;
            j["jsonrpc"] = "2.0";
            j["error"]["code"] = e_code;
            j["error"]["data"] = e_msg;

            switch (e_code) {
                case RPCErrorCode::PARSE_ERROR: {
                    j["id"] = nullptr;
                    j["error"]["message"] = "Parse error";
                    rpc_res.status_code = 500;
                    break;
                }
                case RPCErrorCode::INVALID_REQUEST: {
                    j["id"] = nullptr;
                    j["error"]["message"] = "Invalid Request";
                    rpc_res.status_code = 400;
                    break;
                }
                case RPCErrorCode::METHOD_NOT_FOUND: {
                    j["id"] = id;
                    j["error"]["message"] = "Method not found";
                    rpc_res.status_code = 404;
                    break;
                }
                case RPCErrorCode::INVALID_PARAMS: {
                    j["id"] = id;
                    j["error"]["message"] = "Invalid params";
                    rpc_res.status_code = 500;
                    break;
                }
                case RPCErrorCode::INTERNAL_ERROR: {
                    j["id"] = id;
                    j["error"]["message"] = "Internal error";
                    rpc_res.status_code = 500;
                    break;
                }
                case RPCErrorCode::SERVER_ERROR: {
                    j["id"] = nullptr;
                    j["error"]["message"] = "Server error";
                    rpc_res.status_code = 500;
                    break;
                }
            }

            rpc_res.result = j.dump();
            return rpc_res;
        }

        RPCResult exec_rpc(const std::string& body, const ASPAPISet& api_set) {
            RPCResult rpc_res;
            rpc_res.status_code = 200;

            nlohmann::json res_j;

            auto req_j = nlohmann::json::parse(body);
            std::string jsonrpc;
            std::string id;

            // [TODO] Maybe we should implement Notification Request?
            // In that case, 'id' must not be present on the request body,
            // and server must not return any response.
            // @ref: https://www.jsonrpc.org/specification#notification

            PARSE_REQUEST(jsonrpc, "jsonrpc", std::string)
            PARSE_REQUEST(id, "id", std::string)

            if (jsonrpc != "2.0") {
                return error_rpc_res("", RPCErrorCode::INVALID_REQUEST, "'jsonrpc' must be '2.0'");
            }
            res_j["jsonrpc"] = jsonrpc;
            res_j["id"] = id;

            switch (req_j.at("method").get<ASPMethod>()) {
                case ASPMethod::GENERAL_TERMINATE: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.general->terminate)
                    break;
                }
                case ASPMethod::MEDIA_TAKE_SNAPSHOT: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.media->take_snapshot)
                    break;
                }
                case ASPMethod::MEDIA_SEEK: {
                    PARSE_PARAMS(req_j, res_j, params, int, int)
                    EXEC_METHOD(res_j, api_set, api_set.media->seek, params)
                    break;
                }
                case ASPMethod::MEDIA_RELATIVE_SEEK: {
                    PARSE_PARAMS(req_j, res_j, params, int, int)
                    EXEC_METHOD(res_j, api_set, api_set.media->relative_seek, params)
                    break;
                }
                case ASPMethod::MEDIA_FRAME_STEP: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.media->frame_step)
                    break;
                }
                case ASPMethod::MEDIA_FRAME_BACK_STEP: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.media->frame_back_step)
                    break;
                }
                case ASPMethod::GUI_GET_WIDGETS: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.gui->get_widgets)
                    break;
                }
                case ASPMethod::GUI_CLICK: {
                    PARSE_PARAMS(req_j, res_j, params, std::string)
                    EXEC_METHOD(res_j, api_set, api_set.gui->click, params)
                    break;
                }
                case ASPMethod::INVALID: {
                    return error_rpc_res(res_j["id"], RPCErrorCode::METHOD_NOT_FOUND, "");
                    break;
                }
                default: {
                }
            }
            rpc_res.result = res_j.dump();
            return rpc_res;
        }

    }
}
