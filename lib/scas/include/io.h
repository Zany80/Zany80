#ifndef SCAS_IO_H
#define SCAS_IO_H

#include <stddef.h>
#include <stdio.h>

// A thin `fread` shim which calls `scas_abort` if the amount read does not
// match the expected size.
void scas_read(void *ptr, size_t size, size_t nmemb, FILE *stream);

#endif // SCAS_IO_H

