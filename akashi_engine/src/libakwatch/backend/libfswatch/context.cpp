#include "./context.h"
#include "../../context.h"
#include "../../item.h"
#include "./config.h"
#include "./util.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

#include <libfswatch/c++/monitor.hpp>

using namespace akashi::core;

namespace akashi {
    namespace watch {

        LFSWatchContext::LFSWatchContext(const WatchConfig& wconfig) : WatchContext(wconfig) {
            m_watch_config = wconfig;

            // [TODO] an access to `watch_dir` might not be thread-safe
            auto config = MonitorConfig::create_default(
                {m_watch_config.include_dir.to_cloned_str()}, LFSWatchContext::on_file_change);

            m_active_monitor = fsw::monitor_factory::create_monitor(
                config.monitor_type, config.paths, config.on_file_change, this);

            // m_active_monitor->set_properties(monitor_properties);
            m_active_monitor->set_allow_overflow(config.allow_overflow);
            if (config.latency > 0) {
                m_active_monitor->set_latency(config.latency);
            }
            m_active_monitor->set_recursive(config.recursive);
            m_active_monitor->set_directory_only(config.directory_only);
            m_active_monitor->set_event_type_filters(config.event_filters);
            m_active_monitor->set_filters(config.filters);
            m_active_monitor->set_follow_symlinks(config.follow_symlinks);
            m_active_monitor->set_watch_access(config.watch_access);
        }

        LFSWatchContext::~LFSWatchContext() {
            this->stop();
            delete m_active_monitor;
        }

        void LFSWatchContext::start(void) {
            if (!m_active_monitor->is_running()) {
                AKLOG_INFON("LFSWatchContext::start() monitor started");
                m_active_monitor->start();
            }
        };

        void LFSWatchContext::stop(void) {
            if (m_active_monitor->is_running()) {
                m_active_monitor->stop();
                AKLOG_INFON("LFSWatchContext::stop() monitor exited");
            }
        };

        void LFSWatchContext::on_file_change(const std::vector<fsw::event>& events, void* ctx) {
            auto watch_ctx = reinterpret_cast<LFSWatchContext*>(ctx);

            std::vector<WatchEvent> watch_events(events.size());

            for (size_t i = 0; i < events.size(); i++) {
                std::string flags_str = "";
                for (const auto flag : events[i].get_flags()) {
                    flags_str += fsw::event::get_event_flag_name(flag) + ", ";
                    // [TODO] path memory leak
                    watch_events[i] = {
                        Path(events[i].get_path().c_str()).to_abspath().to_cloned_str(),
                        to_event_flag(flag)};
                }
                AKLOG_INFO("path: {}, time: {}, flags: {}", events[i].get_path().c_str(),
                           events[i].get_time(), flags_str.c_str());
            }

            WatchEventList evt_list;
            evt_list.events = watch_events.data();
            evt_list.size = watch_events.size();

            if (watch_ctx->watch_config().on_file_change) {
                watch_ctx->watch_config().on_file_change(evt_list);
            }
        }

    }
}
