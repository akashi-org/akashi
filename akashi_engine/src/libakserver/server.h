#pragma once

#include <libakcore/memory.h>

#include <functional>

namespace akashi {
    namespace server {

        struct ASPConfig;
        struct ASPAPISet;

        struct RPCRequest;
        struct HTTPRPCResponse;
        using OnRequest =
            std::function<void(HTTPRPCResponse&, const RPCRequest&, const std::string& req_str)>;

        struct NativeServerHandle;

        class ServerHandle final {
          public:
            explicit ServerHandle(NativeServerHandle* handle) : m_native_handle(handle){};
            virtual ~ServerHandle(){};
            void exit();
            void set_handle(NativeServerHandle* handle) noexcept(false);

          private:
            core::borrowed_ptr<NativeServerHandle> m_native_handle;
        };

        void init_renderer_asp_server(const ASPAPISet&& api_set);

        // [XXX] ServerHandle does not outlive after this function returned
        void init_kernel_asp_server(ServerHandle& handle, const ASPConfig& config,
                                    const OnRequest& on_request);

    }
}
