#ifndef STRINGOP_H
#define STRINGOP_H
#include "list.h"

#include <Zany80/internal/dllports.h>

#ifdef __cplusplus
extern "C" {
#endif

ZANY_DLL char *strip_comments(char *str);
ZANY_DLL list_t *split_string(const char *str, const char *delims);
ZANY_DLL char *code_strchr(const char *string, char delimiter);
ZANY_DLL char *code_strstr(const char *haystack, const char *needle);
ZANY_DLL int unescape_string(char *string);
/// Can only be used on lists obtained via the Zany80 API!
ZANY_DLL void free_flat_list(list_t *list);

#ifndef strcasecmp
#ifdef _WIN32
#include <string.h>
#define strcasecmp stricmp
#else
int strcasecmp(const char* s1, const char* s2) __attribute__((weak));
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
