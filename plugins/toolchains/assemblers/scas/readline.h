#ifndef READLINE_H
#define READLINE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

char *read_line(FILE *file);
char *read_line_s(const char *input, int *offset);

#ifdef __cplusplus
}
#endif

#endif
