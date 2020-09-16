#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

typedef enum {
    L_SILENT = 0,
    L_ERROR = 1,
    L_INFO = 2,
    L_DEBUG = 3,
} scas_log_importance_t;

void scas_log_init(scas_log_importance_t verbosity);
void scas_log(scas_log_importance_t verbosity, char* format, ...);
void scas_abort(char* format, ...);
void scas_log_indent();
void scas_log_deindent();
void scas_log_set_colors(bool enabled);

#endif
