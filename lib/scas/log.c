#include "log.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

static scas_log_importance_t v = 0;
static unsigned indent = 0;
static bool colored = true;

const char *verbosity_colors[] = {
	"", // L_SILENT
	"\x1B[1;31m", // L_ERROR
	"\x1B[1;34m", // L_INFO
	"\x1B[1;30m", // L_DEBUG
};

void scas_log_init(scas_log_importance_t verbosity) {
	v = verbosity;
}

void scas_log_set_colors(bool _colored) {
	colored = _colored;
}

void scas_log_indent() {
	++indent;
}

void scas_log_deindent() {
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

void scas_log(scas_log_importance_t verbosity, char* format, ...) {
	if (verbosity <= v && verbosity >= 0) {
		size_t c = verbosity;
		if (c > sizeof(verbosity_colors) / sizeof(char *)) {
			c = sizeof(verbosity_colors) / sizeof(char *) - 1;
		}
		if (colored) {
			fprintf(stderr, "%s", verbosity_colors[c]);
		}
		if (verbosity == L_DEBUG || verbosity == L_INFO) {
			for (unsigned i = 0; i < indent; ++i) {
				fprintf(stderr, "  ");
			}
		}
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "%s\n", colored ? "\x1B[0m" : "");
	}
}
