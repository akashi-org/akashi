#pragma once

#include <libakcore/path.h>
#include <functional>
#include <vector>

namespace akashi {
    namespace watch {

        enum class WatchEventFlag { NONE = -1, CREATED = 0, REMOVED, UPDATED, OTHER };

        struct WatchEvent {
            const char* file_path = nullptr;
            WatchEventFlag flag = WatchEventFlag::NONE;
        };

        struct WatchEventList {
            WatchEvent* events;
            size_t size;
        };

        struct WatchConfig {
            core::Path include_dir = core::Path("");
            std::function<void(const WatchEventList& evt_list)> on_file_change;
        };

    }
}
