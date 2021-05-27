#pragma once

#include <libakstate/akstate.h>

#include <string>
#include <vector>
#include <variant>

namespace akashi {
    namespace server {

        struct ASPAPISet;

        enum ASPMethod {
            INVALID = -1,
            GENERAL_EVAL = 101,
            GENERAL_TERMINATE,
            MEDIA_TAKE_SNAPSHOT = 201,
            MEDIA_TOGGLE_FULLSCREEN,
            MEDIA_SEEK,
            MEDIA_RELATIVE_SEEK,
            MEDIA_FRAME_STEP,
            MEDIA_FRAME_BACK_STEP,
            MEDIA_CURRENT_TIME,
            MEDIA_CHANGE_PLAYSTATE,
            MEDIA_CHANGE_PLAYVOLUME,
            GUI_GET_WIDGETS = 301,
            GUI_CLICK,
        };

        template <ASPMethod method_ = ASPMethod::INVALID>
        struct RPCRequestParams {};

        template <>
        struct RPCRequestParams<ASPMethod::GENERAL_EVAL> {
            std::string fpath;
            std::string elem_name;
        };

        template <>
        struct RPCRequestParams<ASPMethod::MEDIA_SEEK> {
            int num;
            int den;
        };

        template <>
        struct RPCRequestParams<ASPMethod::MEDIA_RELATIVE_SEEK> {
            double ratio;
        };

        template <>
        struct RPCRequestParams<ASPMethod::MEDIA_CHANGE_PLAYSTATE> {
            state::PlayState play_state;
        };

        template <>
        struct RPCRequestParams<ASPMethod::MEDIA_CHANGE_PLAYVOLUME> {
            double volume;
        };

        template <>
        struct RPCRequestParams<ASPMethod::GUI_CLICK> {
            std::string widget_name;
        };

        using RPCRequestParamsTypes =
            std::variant<RPCRequestParams<>, RPCRequestParams<GENERAL_EVAL>,
                         RPCRequestParams<MEDIA_SEEK>, RPCRequestParams<MEDIA_RELATIVE_SEEK>,
                         RPCRequestParams<MEDIA_CHANGE_PLAYSTATE>,
                         RPCRequestParams<MEDIA_CHANGE_PLAYVOLUME>, RPCRequestParams<GUI_CLICK>>;

        struct RPCRequest {
            std::string jsonrpc;
            std::string id;
            ASPMethod method;
            RPCRequestParamsTypes params;
        };

        enum RPCErrorCode {
            PARSE_ERROR = -32700,
            INVALID_REQUEST = -32600,
            METHOD_NOT_FOUND = -32601,
            INVALID_PARAMS = -32602,
            INTERNAL_ERROR = -32603,
            SERVER_ERROR = -32000
        };

        using RPCResultTypes =
            std::variant<bool, std::string, std::vector<std::string>, std::vector<int64_t>>;

        struct RPCResultObject {
            int type_id; // variant index for RPCRequestParamsTypes
            RPCResultTypes value;
        };

        struct RPCErrorObject {
            RPCErrorCode code;
            std::string message;
            std::string data;
        };

        struct RPCResponse {
            std::string jsonrpc;
            std::string id;
            std::variant<RPCResultObject, RPCErrorObject> payload;
        };

        struct HTTPRPCResponse {
            std::string response_str;
            int status_code;
        };

        HTTPRPCResponse success_rpc_res(const std::string& id,
                                        const RPCResultTypes& result) noexcept(false);

        HTTPRPCResponse error_rpc_res(const std::string& id, const RPCErrorCode& e_code,
                                      const std::string& e_msg) noexcept(false);

        HTTPRPCResponse exec_rpc(const std::string& body, const ASPAPISet& api_set) noexcept(false);

        RPCRequest parse_rpc_req(const std::string& body) noexcept(false);

        RPCResponse parse_rpc_res(const std::string& body) noexcept(false);
    }
}
