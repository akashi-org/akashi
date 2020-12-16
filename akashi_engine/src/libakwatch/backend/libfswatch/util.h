#pragma once

#include "../../item.h"
#include <libfswatch/c/cevent.h>

namespace akashi {
    namespace watch {

        WatchEventFlag to_event_flag(const fsw_event_flag& fsw_flag);

    }
}
