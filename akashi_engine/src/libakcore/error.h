#pragma once

#define CHECK_AK_ERROR(test)                                                                       \
    do {                                                                                           \
        if (!test) {                                                                               \
            return akashi::core::ErrorType::Error;                                                 \
        }                                                                                          \
    } while (0)

#define CHECK_AK_ERROR2(test)                                                                      \
    do {                                                                                           \
        if (!test) {                                                                               \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

namespace akashi {
    namespace core {
        enum class [[nodiscard]] ErrorType{Error = -1, OK};
    }
}

using ak_error_t = akashi::core::ErrorType;
