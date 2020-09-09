#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

int v = 0;
int indent = 0;
int colored = 1;

const char *verbosity_colors[] = {
	"", // L_SILENT
	"\x1B[1;31m", // L_ERROR
	"\x1B[1;34m", // L_INFO
	"\x1B[1;30m", // L_DEBUG
};

void init_log(int verbosity) {
	v = verbosity;
}

void enable_colors() {
	colored = 0;
}

void disable_colors() {
	colored = 0;
}

void indent_log() {
	++indent;
}

void deindent_log() {
	if (indent > 0) {
		--indent;
	}
}

void scas_abort(char *format, ...) {
	fprintf(stderr, "ERROR: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
#ifdef EMSCRIPTEN
	EM_ASM(
			throw 'aborting';
		);
#else
	exit(1);
#endif
}

void scas_log(int verbosity, char* format, ...) {
	if (verbosity <= v && verbosity >= 0) {
		size_t c = verbosity;
		if (c > sizeof(verbosity_colors) / sizeof(char *)) {
			c = sizeof(verbosity_colors) / sizeof(char *) - 1;
		}
		if (colored) {
			fprintf(stderr, verbosity_colors[c]);
		}
		if (verbosity == L_DEBUG || verbosity == L_INFO) {
			int i;
			for (i = 0; i < indent; ++i) {
				fprintf(stderr, "  ");
			}
		}
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		if (colored) {
			fprintf(stderr, "\x1B[0m\n");
		} else {
			fprintf(stderr, "\n");
		}
	}
}
