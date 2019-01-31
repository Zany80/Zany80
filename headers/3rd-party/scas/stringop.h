#ifndef STRINGOP_H
#define STRINGOP_H
#include "list.h"

#ifdef _WIN32
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

char *strip_whitespace(char *str, int *trimmed_start);
char *strip_comments(char *str);
list_t *split_string(const char *str, const char *delims);
void free_flat_list(list_t *list);
char *code_strchr(const char *string, char delimiter);
char *code_strstr(const char *haystack, const char *needle);
int unescape_string(char *string);

#ifndef strcasecmp
int strcasecmp(const char* s1, const char* s2) 
#ifndef _WIN32
__attribute__((weak))
#endif
;
#endif

#ifdef __cplusplus
}
#endif

#endif
