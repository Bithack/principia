#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void tms_storage_set_portable(bool portable);

const char *tms_storage_path(void);
const char *tms_storage_cache_path(void);

void tms_storage_create_dirs(void);

#ifdef __cplusplus
}
#endif
