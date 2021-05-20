#pragma once

#include <functional>
#include <string>
#include <vector>

namespace akashi {
    namespace state {
        enum class PlayState;
    }
    namespace server {

        /* all methods must have a return value */

        class ASPGeneralAPI {
          public:
            virtual ~ASPGeneralAPI(){};
            virtual bool eval(const std::string& fpath, const std::string& elem_name) = 0;
            virtual bool terminate(void) = 0;
        };

        class ASPMediaAPI {
          public:
            virtual ~ASPMediaAPI(){};
            virtual std::string take_snapshot(void) = 0;
            virtual bool toggle_fullscreen(void) = 0;
            virtual bool seek(const int num, const int den) = 0;
            virtual bool relative_seek(const int num, const int den) = 0;
            virtual bool frame_step(void) = 0;
            virtual bool frame_back_step(void) = 0;
            virtual std::vector<int64_t> current_time(void) = 0;
            virtual bool change_playstate(const state::PlayState& play_state) = 0;
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
