#ifndef _FLAGS_H
#define _FLAGS_H
#include "linker.h"

struct output_format {
	char *name;
	format_writer writer;
};

void parse_flag(const char *flag);

#endif
