#include "./logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>
#include <memory>

namespace akashi {
    namespace core {

        constexpr const char AK_LOGGER_NAME[] = "akashi_logger";

        void create_logger(const LogCapabilities& cap) {
            std::vector<spdlog::sink_ptr> sinks;

            auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
            console_sink->set_level(static_cast<spdlog::level::level_enum>(cap.console_log_level));
            console_sink->set_color(spdlog::level::debug, console_sink->white);
            console_sink->set_color(spdlog::level::info, console_sink->green);
            console_sink->set_color(spdlog::level::warn, console_sink->yellow);
            console_sink->set_color(spdlog::level::err, console_sink->red);
            console_sink->set_pattern("%^[%t%v%$");
            sinks.push_back(console_sink);

            if (!cap.log_fpath.empty()) {
                // [TODO] maybe we should check whether the file pointed by log_fpath is
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    cap.log_fpath,
                    false // [XXX] since the log file is shared between processes,
                          // keep it always false.
                );
                file_sink->set_level(spdlog::level::debug); // save logs of all levels
                file_sink->set_pattern("%Y-%m-%d %T.%e [%l] - [%t%v");
                sinks.push_back(file_sink);
            }

            auto logger =
                std::make_shared<spdlog::logger>(AK_LOGGER_NAME, sinks.begin(), sinks.end());

            // set the minimum log level that will trigger automatic flush
            logger->flush_on(spdlog::level::debug);
            if (!cap.log_fpath.empty()) {
                logger->set_level(spdlog::level::debug);
            } else {
                logger->set_level(static_cast<spdlog::level::level_enum>(cap.console_log_level));
            }

            // https://github.com/gabime/spdlog/wiki/2.-Creating-loggers#accessing-loggers-using-spdlogget
            spdlog::register_logger(logger);
            spdlog::set_default_logger(spdlog::get(AK_LOGGER_NAME));
        }

        void destroy_logger() {
            spdlog::drop_all();
            spdlog::drop(AK_LOGGER_NAME);
            spdlog::shutdown();
        }

    }
}
