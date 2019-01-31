#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <list.h>

list_t *read_directory(const char *path);
void report_error(const char *message);

#ifdef __cplusplus
}
#endif
