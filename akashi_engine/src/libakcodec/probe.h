#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ak_fraction_t {
    size_t num = 0;
    size_t den = 1;
} ak_fraction_t;

bool akprobe_get_duration(ak_fraction_t* duration, const char* url);

#ifdef __cplusplus
}
#endif
