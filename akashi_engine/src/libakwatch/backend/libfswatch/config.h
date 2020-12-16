#pragma once

#include <libfswatch/c++/monitor.hpp>

#include <vector>
#include <string>

namespace akashi {
    namespace watch {

        class MonitorConfig {
          public:
            std::vector<std::string> paths;
            fsw_monitor_type monitor_type;
            fsw::FSW_EVENT_CALLBACK* on_file_change;
            bool allow_overflow;
            double latency;
            bool recursive;
            bool directory_only;
            bool follow_symlinks;
            bool watch_access;
            std::vector<fsw::monitor_filter> filters;
            std::vector<fsw_event_type_filter> event_filters;

          public:
            static MonitorConfig create_default(const std::vector<std::string>& paths,
                                                fsw::FSW_EVENT_CALLBACK* on_file_change) {
                MonitorConfig self;

                self.paths = paths;

                // dare to use poll instead of inotify to avoid struggling with too many fsw
                // events
                self.monitor_type = fsw_monitor_type::poll_monitor_type;
                // self.monitor_type = fsw_monitor_type::system_default_monitor_type;

                self.on_file_change = on_file_change;

                self.allow_overflow = true;

                self.latency = -1.0; // valid only if it is positive

                self.recursive = true;

                self.directory_only = true;

                self.follow_symlinks = false;

                self.watch_access = false;

                // [BUG] filters not working properly in libfswatch ver1.14.0.
                // In ver1.9.1, this bug does not arise.
                self.filters = {
                    // [XXX] fswatch includes everything by default.
                    // So, to include only specific format files, you must first exclude all.
                    {.text = ".*",
                     .type = fsw_filter_type::filter_exclude,
                     .case_sensitive = false,
                     .extended = true},
                    {.text = "\\.py$",
                     .type = fsw_filter_type::filter_include,
                     .case_sensitive = true,
                     .extended = true},
                };

                self.event_filters = {
                    {fsw_event_flag::Created},
                    {fsw_event_flag::Updated},
                    {fsw_event_flag::Removed},
                    // {fsw_event_flag::Renamed}, // this event cannot be caught by poll monitor
                    {fsw_event_flag::Overflow},
                };

                return self;
            }
        };

    }

}
