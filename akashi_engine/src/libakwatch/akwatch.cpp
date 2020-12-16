#include "./akwatch.h"
#include "./context.h"
#include "./item.h"

#include "./backend/libfswatch.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace watch {

        AKWatch::AKWatch(const WatchConfig& config) {
            owned_ptr<WatchContext> ctx(new LFSWatchContext(config));
            m_watch_ctx = std::move(ctx);
        }

        AKWatch::~AKWatch() {}

        void AKWatch::start(void) { m_watch_ctx->start(); }

        void AKWatch::stop(void) { m_watch_ctx->stop(); }

    }

}
