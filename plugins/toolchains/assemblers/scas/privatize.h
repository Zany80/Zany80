#ifndef PRIVATIZE_H
#define PRIVATIZE_H
#include <stdio.h>
#include "list.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" {
#endif
void privatize_symbols(object_t *o);

#ifdef __cplusplus
}
#endif

#endif
