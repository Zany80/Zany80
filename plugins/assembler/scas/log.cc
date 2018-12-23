#include "scas.h"

#include <log.h>
#include <runtime.h>
#include <string.h>

extern StringBuilder *out;

int v = 0;
int indent = 0;

void scas_log(int verbosity, const char* format, ...) {
	if (verbosity > v)
		return;
	if (verbosity == L_DEBUG || verbosity == L_INFO) {
		int i;
		for (i = 0; i < indent; ++i) {
			out->Append("\t");
		}
	}
	va_list args;
	va_start(args, format);
	out->AppendFormatVAList(strlen(format) * 6, format, args);
	out->Append("\n");
	va_end(args);
}

void init_log(int verbosity) {
	v = verbosity;
}

void scas::setVerbosity(int verbosity) {
	if (verbosity > 1)
		verbosity--;
	Log::Dbg("[scas] verbosity is at %d\n", verbosity);
	scas_runtime.verbosity = v = verbosity;
}

// Ignore colors
void enable_colors() {}
void disable_colors() {}

void indent_log() {
	++indent;
}

void deindent_log() {
	if (indent > 0) {
		--indent;
	}
}
