#pragma once

namespace akashi {
    namespace server {

        struct ASPConfig;
        struct ASPAPISet;

        void init_asp_server(const ASPConfig& config, const ASPAPISet&& api_set);

    }
}
