#pragma once

#include <libakcore/memory.h>

namespace akashi {
    namespace watch {
        class WatchContext;
        struct WatchConfig;
        class AKWatch {
          public:
            explicit AKWatch(const WatchConfig& config);
            virtual ~AKWatch();

            void start(void);
            void stop(void);

          private:
            core::owned_ptr<WatchContext> m_watch_ctx;
        };

    }
}
