#ifndef _SCAS_FORMAT_H
#define _SCAS_FORMAT_H

#include <stdint.h>

void format(void (*putc)(char), uintmax_t (*getarg)(size_t size), const char *fmt);

#endif
