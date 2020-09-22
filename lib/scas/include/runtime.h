#ifndef GLOBALS_H
#define GLOBALS_H
#include <stdbool.h>
#include "list.h"
#include "linker.h"
#include "enums.h"

struct runtime_options {
	bool explicit_export;				/* -fexplicit-export */
	bool explicit_import;				/* -fexplicit-import */
	bool auto_relocation;				/* -fauto-relocation */
	bool remove_unused_functions;		/* -fremove-unused-funcs */
	format_writer output_format;		/* -fformat=... */
	char *prog_name_8xp;				/* -f8xp-name=... */
	bool prog_protected_8xp;			/* -f8xp-protected */
	bool prog_archived_8xp;				/* -f8xp-archived */
	uint64_t origin;					/* -forigin-... */
};

struct runtime {
	char *arch;
	jobs_t jobs;
	list_t *macros;
	output_type_t output_type;
	list_t *input_files, *input_names;
	FILE *output_file;
	char *output_extension;
	char *listing_file;
	char *symbol_file;
	char *include_path;
	char *linker_script;
	int verbosity;
	struct runtime_options options;
};

extern struct runtime scas_runtime;

#endif
