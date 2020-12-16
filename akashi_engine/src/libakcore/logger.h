#pragma once

#include <spdlog/spdlog.h>
#include <string>

#define FORMAT_MSG(msg) ("/{}]({}:{}) " msg)

#define AKLOG_LOG(lvl, msg, ...)                                                                   \
    do {                                                                                           \
        static_assert(lvl != akashi::core::LogLevel::OFF, "LogLevel::OFF is not supported");       \
        SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(),                                           \
                           static_cast<spdlog::level::level_enum>(lvl), FORMAT_MSG(msg),           \
                           akashi::core::detail::get_log_tag_str(__FILE__),                        \
                           akashi::core::detail::to_module_path(__FILE__), __LINE__, __VA_ARGS__); \
    } while (0)

#define AKLOG_LOGN(lvl, msg)                                                                       \
    do {                                                                                           \
        static_assert(lvl != akashi::core::LogLevel::OFF, "LogLevel::OFF is not supported");       \
        SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(),                                           \
                           static_cast<spdlog::level::level_enum>(lvl), FORMAT_MSG(msg),           \
                           akashi::core::detail::get_log_tag_str(__FILE__),                        \
                           akashi::core::detail::to_module_path(__FILE__), __LINE__);              \
    } while (0)

#define AKLOG_DEBUG(msg, ...) AKLOG_LOG(akashi::core::LogLevel::DEBUG, msg, __VA_ARGS__)
#define AKLOG_DEBUGN(msg) AKLOG_LOGN(akashi::core::LogLevel::DEBUG, msg)
#define AKLOG_INFO(msg, ...) AKLOG_LOG(akashi::core::LogLevel::INFO, msg, __VA_ARGS__)
#define AKLOG_INFON(msg) AKLOG_LOGN(akashi::core::LogLevel::INFO, msg)
#define AKLOG_WARN(msg, ...) AKLOG_LOG(akashi::core::LogLevel::WARN, msg, __VA_ARGS__)
#define AKLOG_WARNN(msg) AKLOG_LOGN(akashi::core::LogLevel::WARN, msg)
#define AKLOG_ERROR(msg, ...) AKLOG_LOG(akashi::core::LogLevel::ERROR, msg, __VA_ARGS__)
#define AKLOG_ERRORN(msg) AKLOG_LOGN(akashi::core::LogLevel::ERROR, msg)

namespace akashi {
    namespace core {

        enum class LogLevel { DEBUG = 1, INFO, WARN, ERROR, OFF };

        struct LogCapabilities {
            LogLevel console_log_level = LogLevel::WARN;
            std::string log_fpath;
        };

        void create_logger(const LogCapabilities& cap);

        void destroy_logger();

        namespace detail {

            constexpr inline int64_t contains_char(const char* str, const char* tar,
                                                   int64_t pos = 0, bool seq = false) {
                if (*tar == '\0') {
                    return pos;
                }
                while (*str) {
                    if (*str == *tar) {
                        int64_t res = contains_char(str + 1, tar + 1, pos, true);
                        if (res > 0) {
                            return res;
                        }
                    }
                    if (seq) {
                        return -1;
                    }
                    str++;
                    pos++;
                }
                return -1;
            }

            constexpr inline const char* substr_char(const char* str, const size_t pos) {
                size_t cnt = 0;
                while (*str && cnt < pos) {
                    str++;
                    cnt++;
                }
                return str;
            }

            constexpr inline const char* to_module_path(const char* str) {
                int64_t n = contains_char(str, "src/");
                if (n > 0) {
                    return substr_char(str, n);
                } else {
                    return str;
                }
            }

            constexpr inline const char* get_log_tag_str(const char* str) {
                if (contains_char(str, "akui") > 0) {
                    return "akui";
                }
                if (contains_char(str, "libakplayer") > 0) {
                    return "akplayer";
                }
                if (contains_char(str, "libakcore") > 0) {
                    return "akcore";
                }
                if (contains_char(str, "libakcodec") > 0) {
                    return "akcodec";
                }
                if (contains_char(str, "libakaudio") > 0) {
                    return "akaudio";
                }
                if (contains_char(str, "libakgraphics") > 0) {
                    return "akgraphics";
                }
                if (contains_char(str, "libakbuffer") > 0) {
                    return "akbuffer";
                }
                if (contains_char(str, "libakeval") > 0) {
                    return "akeval";
                }
                if (contains_char(str, "libakwatch") > 0) {
                    return "akwatch";
                }
                if (contains_char(str, "libakserver") > 0) {
                    return "akserver";
                }
                if (contains_char(str, "libakstate") > 0) {
                    return "akstate";
                }
                if (contains_char(str, "libakevent") > 0) {
                    return "akevent";
                }
                return "???";
            }

        }

    }
}
