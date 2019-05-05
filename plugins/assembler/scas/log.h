#ifndef LOG_H
#define LOG_H

typedef enum {
    L_SILENT = 0,
    L_ERROR = 1,
    L_INFO = 2,
    L_DEBUG = 3,
} log_importance_t;

#ifdef __cplusplus
extern "C" {
#endif
void init_log(int verbosity);
void scas_log(int verbosity, const char* format, ...);
void scas_abort(const char* format, ...);
void indent_log();
void deindent_log();
void enable_colors();
void disable_colors();
#ifdef __cplusplus
}
#endif

#endif
