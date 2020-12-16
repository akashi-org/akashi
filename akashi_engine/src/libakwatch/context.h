#pragma once

namespace akashi {
    namespace watch {
        struct WatchConfig;
        class WatchContext {
          public:
            explicit WatchContext(const WatchConfig&){};
            virtual ~WatchContext() = default;
            virtual void start(void) = 0;
            virtual void stop(void) = 0;
        };

    }
}
