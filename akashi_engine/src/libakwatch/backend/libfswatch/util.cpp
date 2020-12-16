#include "./util.h"

#include "../../item.h"
#include <libfswatch/c/cevent.h>

namespace akashi {
    namespace watch {

        WatchEventFlag to_event_flag(const fsw_event_flag& fsw_flag) {
            switch (fsw_flag) {
                case fsw_event_flag::Created: {
                    return WatchEventFlag::CREATED;
                }
                case fsw_event_flag::Removed: {
                    return WatchEventFlag::REMOVED;
                }
                case fsw_event_flag::Updated: {
                    return WatchEventFlag::UPDATED;
                }
                default: {
                    return WatchEventFlag::OTHER;
                }
            }
        }

    }
}
