#pragma once

#include <spdlog/spdlog.h>
#include <string>

#define FORMAT_MSG(msg) ("/{}]({}:{}) " msg)

#ifndef NDEBUG
#define ASSERT_LOGTAG()                                                                            \
    ((!std::getenv("AK_LOGTAG")) ||                                                                \
     (std::string(std::getenv("AK_LOGTAG")) ==                                                     \
      std::string(akashi::core::detail::get_log_tag_str(__FILE__))))
#else
#define ASSERT_LOGTAG() (true)
#endif

#define AKLOG_LOG(lvl, msg, ...)                                                                   \
    do {                                                                                           \
        static_assert(lvl != akashi::core::LogLevel::OFF, "LogLevel::OFF is not supported");       \
        if (ASSERT_LOGTAG()) {                                                                     \
            SPDLOG_LOGGER_CALL(                                                                    \
                spdlog::default_logger_raw(), static_cast<spdlog::level::level_enum>(lvl),         \
                FORMAT_MSG(msg), akashi::core::detail::get_log_tag_str(__FILE__),                  \
                akashi::core::detail::to_module_path(__FILE__), __LINE__, __VA_ARGS__);            \
        }                                                                                          \
    } while (0)

#define AKLOG_LOGN(lvl, msg)                                                                       \
    do {                                                                                           \
        static_assert(lvl != akashi::core::LogLevel::OFF, "LogLevel::OFF is not supported");       \
        if (ASSERT_LOGTAG()) {                                                                     \
            SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(),                                       \
                               static_cast<spdlog::level::level_enum>(lvl), FORMAT_MSG(msg),       \
                               akashi::core::detail::get_log_tag_str(__FILE__),                    \
                               akashi::core::detail::to_module_path(__FILE__), __LINE__);          \
        }                                                                                          \
    } while (0)

#define AKLOG_DEBUG(msg, ...) AKLOG_LOG(akashi::core::LogLevel::DEBUG, msg, __VA_ARGS__)
#define AKLOG_DEBUGN(msg) AKLOG_LOGN(akashi::core::LogLevel::DEBUG, msg)
#define AKLOG_INFO(msg, ...) AKLOG_LOG(akashi::core::LogLevel::INFO, msg, __VA_ARGS__)
#define AKLOG_INFON(msg) AKLOG_LOGN(akashi::core::LogLevel::INFO, msg)
#define AKLOG_WARN(msg, ...) AKLOG_LOG(akashi::core::LogLevel::WARN, msg, __VA_ARGS__)
#define AKLOG_WARNN(msg) AKLOG_LOGN(akashi::core::LogLevel::WARN, msg)
#define AKLOG_ERROR(msg, ...) AKLOG_LOG(akashi::core::LogLevel::ERROR, msg, __VA_ARGS__)
#define AKLOG_ERRORN(msg) AKLOG_LOGN(akashi::core::LogLevel::ERROR, msg)

#define AKLOG_RLOG(lvl, ...)                                                                       \
    do {                                                                                           \
        switch (lvl) {                                                                             \
            case akashi::core::LogLevel::DEBUG: {                                                  \
                AKLOG_DEBUG(__VA_ARGS__);                                                          \
                break;                                                                             \
            }                                                                                      \
            case akashi::core::LogLevel::INFO: {                                                   \
                AKLOG_INFO(__VA_ARGS__);                                                           \
                break;                                                                             \
            }                                                                                      \
            case akashi::core::LogLevel::WARN: {                                                   \
                AKLOG_WARN(__VA_ARGS__);                                                           \
                break;                                                                             \
            }                                                                                      \
            case akashi::core::LogLevel::ERROR: {                                                  \
                AKLOG_ERROR(__VA_ARGS__);                                                          \
                break;                                                                             \
            }                                                                                      \
            default: {                                                                             \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define AKLOG_RLOGN(lvl, ...)                                                                      \
    do {                                                                                           \
        switch (lvl) {                                                                             \
            case akashi::core::LogLevel::DEBUG: {                                                  \
                AKLOG_DEBUGN(__VA_ARGS__);                                                         \
                break;                                                                             \
            }                                                                                      \
            case akashi::core::LogLevel::INFO: {                                                   \
                AKLOG_INFON(__VA_ARGS__);                                                          \
                break;                                                                             \
            }                                                                                      \
            case akashi::core::LogLevel::WARN: {                                                   \
                AKLOG_WARNN(__VA_ARGS__);                                                          \
                break;                                                                             \
            }                                                                                      \
            case akashi::core::LogLevel::ERROR: {                                                  \
                AKLOG_ERRORN(__VA_ARGS__);                                                         \
                break;                                                                             \
            }                                                                                      \
            default: {                                                                             \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define AKPRIV_RETURN_TAGSTR(ret_str)                                                              \
    if (contains_char(str, ret_str) > 0) {                                                         \
        return ret_str;                                                                            \
    }

#define AKPRIV_RETURN_TAGSTR2(match_str, ret_str)                                                  \
    if (contains_char(str, match_str) > 0) {                                                       \
        return ret_str;                                                                            \
    }

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
                AKPRIV_RETURN_TAGSTR("akrenderer");
                AKPRIV_RETURN_TAGSTR("akencoder");
                AKPRIV_RETURN_TAGSTR("akkernel");
                AKPRIV_RETURN_TAGSTR2("/akplayer/", "akplayer");

                AKPRIV_RETURN_TAGSTR("libakui");
                AKPRIV_RETURN_TAGSTR("libakplayer");
                AKPRIV_RETURN_TAGSTR("libakcore");
                AKPRIV_RETURN_TAGSTR("libakcodec");
                AKPRIV_RETURN_TAGSTR("libakaudio");
                AKPRIV_RETURN_TAGSTR("libakgraphics");
                AKPRIV_RETURN_TAGSTR("libakvgfx");
                AKPRIV_RETURN_TAGSTR("libakbuffer");
                AKPRIV_RETURN_TAGSTR("libakeval");
                AKPRIV_RETURN_TAGSTR("libakwatch");
                AKPRIV_RETURN_TAGSTR("libakserver");
                AKPRIV_RETURN_TAGSTR("libakstate");
                AKPRIV_RETURN_TAGSTR("libakevent");
                return "???";
            }

        }

    }
}
