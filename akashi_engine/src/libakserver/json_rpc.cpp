#include "./json_rpc.h"
#include "./api.h"

#include <libakcore/logger.h>
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
        auto value = std::apply([&api_set](auto&... args) { return method(args...); }, (params));  \
        (res_j)["result"]["type_id"] = static_cast<RPCResultTypes>(value).index();                 \
        (res_j)["result"]["value"] = value;                                                        \
    } catch (const std::exception& e) {                                                            \
        return error_rpc_res((res_j)["id"], RPCErrorCode::INTERNAL_ERROR, e.what());               \
    }

#define EXEC_METHOD_NO_PARAMS(res_j, api_set, method)                                              \
    try {                                                                                          \
        auto value = method();                                                                     \
        (res_j)["result"]["type_id"] = static_cast<RPCResultTypes>(value).index();                 \
        (res_j)["result"]["value"] = value;                                                        \
    } catch (const std::exception& e) {                                                            \
        return error_rpc_res((res_j)["id"], RPCErrorCode::INTERNAL_ERROR, e.what());               \
    }

namespace akashi {
    namespace state {
        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(PlayState, {
            {PlayState::NONE, nullptr},
            {PlayState::STOPPED, "stopped"},
            {PlayState::PAUSED, "paused"},
            {PlayState::PLAYING, "playing"}
        })
        // clang-format on
    }
    namespace server {

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(ASPMethod, {
            {INVALID, nullptr},
            {GENERAL_EVAL, "general/eval"},
            {GENERAL_TERMINATE, "general/terminate"},
            {MEDIA_TAKE_SNAPSHOT, "media/take_snapshot"},
            {MEDIA_TOGGLE_FULLSCREEN, "media/toggle_fullscreen"},
            {MEDIA_SEEK, "media/seek"},
            {MEDIA_RELATIVE_SEEK, "media/relative_seek"},
            {MEDIA_FRAME_STEP, "media/frame_step"},
            {MEDIA_FRAME_BACK_STEP, "media/frame_back_step"},
            {MEDIA_CURRENT_TIME, "media/current_time"},
            {MEDIA_CHANGE_PLAYSTATE, "media/change_playstate"},
            {GUI_GET_WIDGETS, "gui/get_widgets"},
            {GUI_CLICK, "gui/click"}
        })
        // clang-format on

        template <typename T>
        void parse_rpc_request_params(const nlohmann::json& j, T& params) noexcept(false) {
            params = j.at("params").get<T>();
        }

        HTTPRPCResponse success_rpc_res(const std::string& id,
                                        const RPCResultTypes& result) noexcept(false) {
            HTTPRPCResponse rpc_res;

            nlohmann::json j;
            j["jsonrpc"] = "2.0";
            j["id"] = id;

            j["result"]["type_id"] = result.index();
            if (auto v = std::get_if<bool>(&result)) {
                j["result"]["value"] = *v;
            } else if (auto v = std::get_if<std::string>(&result)) {
                j["result"]["value"] = *v;
            } else if (auto v = std::get_if<std::vector<std::string>>(&result)) {
                j["result"]["value"] = *v;
            } else if (auto v = std::get_if<std::vector<int64_t>>(&result)) {
                j["result"]["value"] = *v;
            } else {
                AKLOG_ERROR("Invalid RPCResultType found: {}", result.index());
                throw std::runtime_error("Invalid RPCResultType found");
            }

            rpc_res.status_code = 200;
            rpc_res.response_str = j.dump();

            return rpc_res;
        }

        HTTPRPCResponse error_rpc_res(const std::string& id, const RPCErrorCode& e_code,
                                      const std::string& e_msg) {
            // @ref: https://www.jsonrpc.org/specification
            // @ref: https://www.jsonrpc.org/historical/json-rpc-over-http.html

            HTTPRPCResponse rpc_res;

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

            rpc_res.response_str = j.dump();
            return rpc_res;
        }

        HTTPRPCResponse exec_rpc(const std::string& body, const ASPAPISet& api_set) {
            HTTPRPCResponse rpc_res;
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
                case ASPMethod::GENERAL_EVAL: {
                    PARSE_PARAMS(req_j, res_j, params, std::string, std::string)
                    EXEC_METHOD(res_j, api_set, api_set.general->eval, params)
                    break;
                }
                case ASPMethod::GENERAL_TERMINATE: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.general->terminate)
                    break;
                }
                case ASPMethod::MEDIA_TAKE_SNAPSHOT: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.media->take_snapshot)
                    break;
                }
                case ASPMethod::MEDIA_TOGGLE_FULLSCREEN: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.media->toggle_fullscreen)
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
                case ASPMethod::MEDIA_CURRENT_TIME: {
                    EXEC_METHOD_NO_PARAMS(res_j, api_set, api_set.media->current_time)
                    break;
                }
                case ASPMethod::MEDIA_CHANGE_PLAYSTATE: {
                    PARSE_PARAMS(req_j, res_j, params, state::PlayState)
                    EXEC_METHOD(res_j, api_set, api_set.media->change_playstate, params)
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
            rpc_res.response_str = res_j.dump();
            return rpc_res;
        }

        RPCRequest parse_rpc_req(const std::string& body) noexcept(false) {
            // This function does not validate the request fully.
            // Such Validation is responsible for the caller.
            RPCRequest req;

            auto req_j = nlohmann::json::parse(body);

            req.jsonrpc = req_j.at("jsonrpc").get<std::string>();
            req.id = req_j.at("id").get<std::string>();
            req.method = req_j.at("method").get<ASPMethod>();

            switch (req.method) {
                case ASPMethod::GENERAL_EVAL: {
                    auto params = req_j.at("params").get<std::tuple<std::string, std::string>>();
                    req.params = RPCRequestParams<ASPMethod::GENERAL_EVAL>{std::get<0>(params),
                                                                           std::get<1>(params)};
                    break;
                }
                case ASPMethod::MEDIA_SEEK: {
                    auto params = req_j.at("params").get<std::tuple<int, int>>();
                    req.params = RPCRequestParams<ASPMethod::MEDIA_SEEK>{std::get<0>(params),
                                                                         std::get<1>(params)};
                    break;
                }
                case ASPMethod::MEDIA_RELATIVE_SEEK: {
                    auto params = req_j.at("params").get<std::tuple<int, int>>();
                    req.params = RPCRequestParams<ASPMethod::MEDIA_RELATIVE_SEEK>{
                        std::get<0>(params), std::get<1>(params)};
                    break;
                }
                case ASPMethod::MEDIA_CHANGE_PLAYSTATE: {
                    auto params = req_j.at("params").get<std::tuple<state::PlayState>>();
                    req.params =
                        RPCRequestParams<ASPMethod::MEDIA_CHANGE_PLAYSTATE>{std::get<0>(params)};
                    break;
                }
                case ASPMethod::GUI_CLICK: {
                    auto params = req_j.at("params").get<std::tuple<std::string>>();
                    req.params = RPCRequestParams<ASPMethod::GUI_CLICK>{std::get<0>(params)};
                    break;
                }
                default: {
                }
            }

            return req;
        }

        RPCResponse parse_rpc_res(const std::string& body) noexcept(false) {
            RPCResponse res;

            auto res_j = nlohmann::json::parse(body);

            res.jsonrpc = res_j.at("jsonrpc").get<std::string>();
            res.id = res_j.at("id").get<std::string>();

            if (res_j.contains("result")) {
                RPCResultObject res_obj;
                res_obj.type_id = res_j["result"]["type_id"].get<int>();
                switch (res_obj.type_id) {
                    case 0: {
                        res_obj.value = res_j["result"]["value"].get<bool>();
                        break;
                    }
                    case 1: {
                        res_obj.value = res_j["result"]["value"].get<std::string>();
                        break;
                    }
                    case 2: {
                        res_obj.value = res_j["result"]["value"].get<std::vector<std::string>>();
                        break;
                    }
                    case 3: {
                        res_obj.value = res_j["result"]["value"].get<std::vector<int64_t>>();
                        break;
                    }
                    default: {
                        throw std::runtime_error("Invalid type_id found");
                    }
                }
                res.payload = res_obj;
            } else if (res_j.contains("error")) {
                RPCErrorObject err_obj;
                err_obj.code = res_j["error"]["code"].get<RPCErrorCode>();
                err_obj.message = res_j["error"]["message"].get<std::string>();
                if (res_j["error"].contains("data")) {
                    err_obj.data = res_j["error"]["data"].get<std::string>();
                }
                res.payload = err_obj;
            } else {
                throw std::runtime_error("Invalid body found");
            }

            return res;
        }
    }
}
