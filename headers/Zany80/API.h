#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <scas/list.h>
#include "internal/dllports.h"

// Returns a list of files in the specified directory.
// Must be freed using free_flat_list, which frees all of the strings
// plus the list itselfs
ZANY_DLL list_t *zany_read_directory(const char *path);
// Returns a string containing the path of the Zany80 installation
// Must be free()d by the caller!
ZANY_DLL char *zany_root_folder();
ZANY_DLL void zany_report_error(const char *message);

typedef enum {
    ZL_EXTRADEBUG, ZL_DEBUG, ZL_INFO, ZL_WARN, ZL_ERROR
} zany_loglevel;

ZANY_DLL void zany_log(zany_loglevel level, const char *format, ...);

#ifdef __cplusplus
}
#endif
