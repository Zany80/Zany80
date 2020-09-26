#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scas/include/log.h"
#include "scas/include/runtime.h"
#include "scas/include/bin.h"
#include "scas/include/instructions.h"
#include "scas/include/assembler.h"
#include "z80_tab.h"
#include "serial.h"

struct runtime scas_runtime;

static jmp_buf env;
static scas_log_importance_t v = 0;
static unsigned indent = 0;
static bool colored = true;

const char *verbosity_colors[] = {
	"", // L_SILENT
	"[FF5555]", // L_ERROR
	"[0000AA]", // L_INFO
	"[3AAAAA]", // L_DEBUG
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

void scas_log(scas_log_importance_t verbosity, char* format, ...) {
	if (verbosity <= v && verbosity != L_SILENT) {
		size_t c = verbosity;
		if (c > sizeof(verbosity_colors) / sizeof(char *)) {
			c = sizeof(verbosity_colors) / sizeof(char *) - 1;
		}
		if (colored) {
			serial_write_all(verbosity_colors[c], 8);
		}
		if (verbosity == L_DEBUG || verbosity == L_INFO) {
			for (size_t i = 0; i < indent * 2; ++i) {
				serial_write(' ');
			}
		}
		va_list args;
		va_start(args, format);
		int len = vsnprintf(NULL, 0, format, args);
		va_end(args);
		va_start(args, format);
		char *buf = malloc(len + 1);
		vsnprintf(buf, len + 1, format, args);
		va_end(args);
		serial_write_all(buf, len);
		if (colored) {
			serial_write_all("[FFFFFF]", 8);
		}
		serial_write('\n');
	}
}

void init_scas_runtime() {
	scas_runtime.arch = "z80";
	scas_runtime.jobs = LINK | ASSEMBLE;
	scas_runtime.macros = create_list();
	scas_runtime.output_type = EXECUTABLE;
	scas_runtime.input_files = create_list();
	scas_runtime.input_names = create_list();
	scas_runtime.output_file = NULL;
	scas_runtime.output_extension = "bin";
	scas_runtime.listing_file = NULL;
	scas_runtime.symbol_file = NULL;
	scas_runtime.include_path = "./";
	scas_runtime.linker_script = NULL;
	scas_runtime.verbosity = L_SILENT;

	scas_runtime.options.explicit_export = true;
	scas_runtime.options.explicit_import = true;
	scas_runtime.options.auto_relocation = false;
	scas_runtime.options.remove_unused_functions = true;
	scas_runtime.options.output_format = output_bin;
	scas_runtime.options.origin = 0;
}

void scas_abort(char *format, ...) {
	const char msg[] = "ERROR: ";
	serial_write_all(msg, sizeof(msg) - 1);
	va_list args;
	va_start(args, format);
	int len = vsnprintf(NULL, 0, format, args);
	va_end(args);
	va_start(args, format);
	char *buf = malloc(len + 1);
	vsnprintf(buf, len + 1, format, args);
	va_end(args);
	serial_write_all(buf, len);
	serial_write('\n');
	longjmp(env, 1);
}

void validate_scas_runtime() {
	if (scas_runtime.input_files->length == 0) {
	        scas_abort("No input files given");
	}
	if (!scas_runtime.output_file) {
		scas_abort("No output file given");
	}
}

static instruction_set_t *find_inst() {
	const char *sets_dir = "/usr/share/scas/tables/";
	const char *ext = ".tab";
	FILE *f = fopen(scas_runtime.arch, "r");
	if (!f) {
		char *path = malloc(strlen(scas_runtime.arch) + strlen(sets_dir) + strlen(ext) + 1);
		strcpy(path, sets_dir);
		strcat(path, scas_runtime.arch);
		strcat(path, ext);
		f = fopen(path, "r");
		free(path);
		if (!f) {
			if (!strcmp(scas_runtime.arch, "z80")) {
				// Load from internal copy
				scas_log(L_DEBUG, "Loading internal z80 table...");
				instruction_set_t *set = load_instruction_set_s(z80_tab);
				return set;
			} else {
				scas_abort("Unknown architecture: %s", scas_runtime.arch);
			}
		}
	}
	instruction_set_t *set = load_instruction_set(f);
	fclose(f);
	return set;
}

static list_t *split_include_path() {
	list_t *list = create_list();
	int i, j;
	for (i = 0, j = 0; scas_runtime.include_path[i]; ++i) {
		if (scas_runtime.include_path[i] == ':' || scas_runtime.include_path[i] == ';') {
			char *s = malloc(i - j + 1);
			strncpy(s, scas_runtime.include_path + j, i - j);
			s[i - j] = '\0';
			j = i + 1;
			list_add(list, s);
		}
	}
	char *s = malloc(i - j + 1);
	strncpy(s, scas_runtime.include_path + j, i - j);
	s[i - j] = '\0';
	list_add(list, s);
	return list;
}

bool scas_assemble(FILE *infile, FILE *outfile) {
	if (setjmp(env) == 0) {
		colored = false;
		v = L_ERROR;
		init_scas_runtime();
		list_add(scas_runtime.input_files, infile);
		list_add(scas_runtime.input_names, "<src>");
		scas_runtime.output_file = outfile;
		validate_scas_runtime();
		instruction_set_t *inst = find_inst();
		if (!inst) {
			scas_abort("Failed to load instruction set!");
		}
		scas_log(L_INFO, "Loaded instruction set: %s", inst->arch);
		list_t *errors = create_list();
		list_t *warnings = create_list();
		list_t *include_path = split_include_path();
		list_t *objects = create_list();
		scas_log(L_INFO, "Assembling input file");
		scas_log_indent();
		assembler_settings_t settings = {
			.include_path = include_path,
			.set = inst,
			.errors = errors,
			.warnings = warnings,
			.macros = scas_runtime.macros,
		};
		object_t *o = assemble(infile, "<src>",&settings);
		fclose(infile);
		scas_log(L_INFO, "Assembler returned %d errors, %d warnings.",
				errors->length, warnings->length);
		list_add(objects, o);
		scas_log_deindent();
		return errors->length == 0;
	}
	else {
		return false;
	}
}

