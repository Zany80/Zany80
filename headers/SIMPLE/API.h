#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "internal/dllports.h"
#include "API/graphics.h"
#include "SIMPLE/scas/list.h"

// Returns a list of files in the specified directory.
// Must be freed using free_flat_list, which frees all of the strings
// plus the list itself
SIMPLE_DLL list_t *simple_read_directory(const char *path);
// Returns a string containing the path of the SIMPLE installation
// Must be free()d by the caller!
SIMPLE_DLL char *h_simple_root_folder();
SIMPLE_DLL void simple_report_error(const char *message);

enum {
    SL_EXTRADEBUG, SL_DEBUG, SL_INFO, SL_WARN, SL_ERROR
};

SIMPLE_DLL void simple_log(int level, const char *format, ...);

SIMPLE_DLL double s_simple_elapsed();

#ifdef __cplusplus
}
#endif
