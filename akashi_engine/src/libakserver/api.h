#pragma once

#include <functional>
#include <string>
#include <vector>

namespace akashi {
    namespace server {

        /* all methods must have a return value */

        class ASPGeneralAPI {
          public:
            virtual ~ASPGeneralAPI(){};
            virtual bool terminate(void) = 0;
        };

        class ASPMediaAPI {
          public:
            virtual ~ASPMediaAPI(){};
            virtual std::string take_snapshot(void) = 0;
            virtual bool seek(const int num, const int den) = 0;
            virtual bool relative_seek(const int num, const int den) = 0;
            virtual bool frame_step(void) = 0;
            virtual bool frame_back_step(void) = 0;
        };

        class ASPGUIAPI {
          public:
            virtual ~ASPGUIAPI(){};
            virtual std::vector<std::string> get_widgets(void) = 0;
            virtual bool click(const std::string& widget_name) = 0;
        };

        struct ASPAPISet {
            ASPGeneralAPI* general = nullptr;
            ASPMediaAPI* media = nullptr;
            ASPGUIAPI* gui = nullptr;
        };

        struct ASPConfig {
            const char* host;
            uint32_t port;
        };

    }
}
