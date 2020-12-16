#include "./watch_loop.h"
#include "../event.h"

#include <libakwatch/akwatch.h>
#include <libakwatch/item.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakstate/akstate.h>

#include <thread>

using namespace akashi::core;

namespace akashi {
    namespace player {

        void WatchLoop::watch_thread(WatchLoopContext ctx, WatchLoop* loop) {
            // [TODO] watch thread
            watch::WatchConfig config;

            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                config.include_dir = ctx.state->m_prop.eval_state.config.include_dir;
            }

            config.on_file_change = [&ctx](const watch::WatchEventList& event_list) {
                ctx.event->emit_hot_reload(event_list);
            };

            owned_ptr<watch::AKWatch> watch(new watch::AKWatch(config));

            loop->set_on_thread_exit(
                [](void* ctx) {
                    auto watch = reinterpret_cast<watch::AKWatch*>(ctx);
                    watch->stop();
                },
                watch.get());

            watch->start();
        }

    }
}
