#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace akashi {
    namespace server {

        struct ASPAPISet;

        enum ASPMethod {
            INVALID = -1,
            GENERAL_TERMINATE = 101,
            MEDIA_TAKE_SNAPSHOT = 201,
            MEDIA_SEEK,
            MEDIA_RELATIVE_SEEK,
            MEDIA_FRAME_STEP,
            MEDIA_FRAME_BACK_STEP,
            GUI_GET_WIDGETS = 301,
            GUI_CLICK,
        };

        // clang-format off
        NLOHMANN_JSON_SERIALIZE_ENUM(ASPMethod, {
            {INVALID, nullptr},
            {GENERAL_TERMINATE, "general/terminate"},
            {MEDIA_TAKE_SNAPSHOT, "media/take_snapshot"},
            {MEDIA_SEEK, "media/seek"},
            {MEDIA_RELATIVE_SEEK, "media/relative_seek"},
            {MEDIA_FRAME_STEP, "media/frame_step"},
            {MEDIA_FRAME_BACK_STEP, "media/frame_back_step"},
            {GUI_GET_WIDGETS, "gui/get_widgets"},
            {GUI_CLICK, "gui/click"}
        })
        // clang-format on

        enum RPCErrorCode {
            PARSE_ERROR = -32700,
            INVALID_REQUEST = -32600,
            METHOD_NOT_FOUND = -32601,
            INVALID_PARAMS = -32602,
            INTERNAL_ERROR = -32603,
            SERVER_ERROR = -32000
        };

        struct RPCResult {
            std::string result;
            int status_code;
        };

        RPCResult error_rpc_res(const std::string& id, const RPCErrorCode& e_code,
                                const std::string& e_msg) noexcept(false);

        RPCResult exec_rpc(const std::string& body, const ASPAPISet& api_set) noexcept(false);

    }
}
