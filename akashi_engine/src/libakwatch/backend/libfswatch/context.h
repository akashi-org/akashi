#pragma once

#include "../../context.h"
#include "../../item.h"

#include <libakcore/memory.h>

#include <vector>

namespace fsw {
    class monitor;
    class event;
}

namespace akashi {
    namespace watch {

        struct WatchConfig;
        class LFSWatchContext : public WatchContext {
          public:
            explicit LFSWatchContext(const WatchConfig& wconfig);
            virtual ~LFSWatchContext();

            void start(void) override;
            void stop(void) override;

            WatchConfig watch_config(void) const { return m_watch_config; }

          private:
            static void on_file_change(const std::vector<fsw::event>& events, void* ctx);

          private:
            WatchConfig m_watch_config;
            fsw::monitor* m_active_monitor;
        };

    }
}
