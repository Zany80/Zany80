#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "internal/dllports.h"
#include "data.h"
#include "graphics.h"
#include "XML.h"
#include "scas/list.h"
#include "scas/stringop.h"
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
