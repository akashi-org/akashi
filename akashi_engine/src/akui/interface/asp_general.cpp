#include "./asp.h"

#include <libakserver/akserver.h>

#include <csignal>
#include <unistd.h>

namespace akashi {
    namespace ui {

        bool ASPGeneralAPIImpl::terminate(void) {
            kill(getpid(), SIGTERM);
            return true;
        }

    }
}
