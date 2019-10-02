#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "SIMPLE/internal/dllports.h"
#include "SIMPLE/API/graphics.h"
#include "SIMPLE/scas/list.h"
#include <stdbool.h>

SIMPLE_DLL void simple_report_error(const char *message);

enum {
    SL_EXTRADEBUG, SL_DEBUG, SL_INFO, SL_WARN, SL_ERROR
};

SIMPLE_DLL void simple_log(int level, const char *format, ...);
SIMPLE_DLL double s_simple_elapsed();

#ifdef __cplusplus
}
#endif
