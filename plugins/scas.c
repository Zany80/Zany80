#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <SIMPLE/API.h>
#include <SIMPLE/Plugin.h>
#include <SIMPLE/scas/list.h>
#include <SIMPLE/scas/stringop.h>

#include "log.h"
#include "runtime.h"
#include "bin.h"
#include "instructions.h"
#include "generated.h"
#include "assembler.h"
#include "errors.h"
#include "objects.h"
#include "merge.h"

#include "setjmp.h"

static char *output;
static size_t out_len, out_cap;
static jmp_buf env;

void free_local_flat_list(list_t *list) {
	int i;
	for (i = 0; i < list->length; ++i) {
		free(list->items[i]);
	}
	list_free(list);
}

int convert(list_t *sources, const char *target, char **buffer) {
	if (target == NULL || sources == NULL) {
		*buffer = strdup("Received null pointer as an argument\n");
		return -1;
	}
	if (sources->length == 0) {
		*buffer = strdup("No input files!");
	}
	output = malloc(1);
	out_len = 0;
	out_cap = 0;
	struct runtime scas_runtime;
	scas_runtime.arch = "embedded";
	scas_runtime.jobs = target[strlen(target) - 1] == 'o' ? (ASSEMBLE) : (ASSEMBLE | LINK);
	scas_runtime.macros = create_list();
	scas_runtime.output_type = EXECUTABLE;
	scas_runtime.input_files = sources;
	scas_runtime.output_file = strdup(target);
	scas_runtime.listing_file = NULL;
	scas_runtime.symbol_file = NULL;
	scas_runtime.include_path = strdup("./");
	scas_runtime.linker_script = NULL;
	scas_runtime.options.explicit_export = false;
	scas_runtime.options.explicit_import = true;
	scas_runtime.options.auto_relocation = false;
	scas_runtime.options.remove_unused_functions = true;
	scas_runtime.options.output_format = output_bin;
	scas_runtime.options.origin = 0;
	instruction_set_t *set = load_instruction_set_s(embedded_table);
	scas_log(L_ERROR, "Instruction set loaded: %s", set->arch);
	list_t *include_path = create_list();
	list_add(include_path, scas_runtime.include_path);
	list_t *errors = create_list();
	list_t *warnings = create_list();
	list_t *objects = create_list();
	int ret;
	if (setjmp(env) == 0) {
		int i;
		for (i = 0; i < scas_runtime.input_files->length; ++i) {
			scas_log(L_INFO, "Assembling input file: '%s'", scas_runtime.input_files->items[i]);
			indent_log();
			FILE *f = fopen(scas_runtime.input_files->items[i], "rb");
			if (!f) {
				scas_abort("Unable to open '%s' for assembly.", scas_runtime.input_files->items[i]);
			}
			char magic[7];
			bool is_object = false;
			if (fread(magic, sizeof(char), 7, f) == 7) {
				if (strncmp("SCASOBJ", magic, 7) == 0) {
					is_object = true;
				}
			}
			fseek(f, 0L, SEEK_SET);

			object_t *o;
			if (is_object) {
				scas_log(L_INFO, "Loading object file '%s'", scas_runtime.input_files->items[i]);
				o = freadobj(f, scas_runtime.input_files->items[i]);
			} else {
				assembler_settings_t settings = {
					.include_path = include_path,
					.set = set,
					.errors = errors,
					.warnings = warnings,
					.macros = scas_runtime.macros,
				};
				o = assemble(f, scas_runtime.input_files->items[i], &settings);
				fclose(f);
				scas_log(L_INFO, "Assembler returned %d errors, %d warnings for '%s'",
						errors->length, warnings->length, scas_runtime.input_files->items[i]);
			}
			list_add(objects, o);
			deindent_log();
		}
		scas_log(L_DEBUG, "Opening output file for writing: %s", scas_runtime.output_file);
		FILE *out = fopen(scas_runtime.output_file, "wb");
		if (!out) {
			scas_abort("Unable to open '%s' for output.", scas_runtime.output_file);
		}

		if ((scas_runtime.jobs & LINK) == LINK) {
			scas_log(L_INFO, "Passing objects to linker");
			linker_settings_t settings = {
				.automatic_relocation = scas_runtime.options.auto_relocation,
				.merge_only = false,
				.errors = errors,
				.warnings = warnings,
				.write_output = scas_runtime.options.output_format
			};
			link_objects(out, objects, &settings);
			fflush(out);
			fclose(out);
			scas_log(L_INFO, "Linker returned %d errors, %d warnings", errors->length, warnings->length);
		} else {
			scas_log(L_INFO, "Skipping linking - writing to object file");
			object_t *merged = merge_objects(objects);
			fwriteobj(out, merged);
			fflush(out);
			fclose(out);
		}
		if (errors->length != 0) {
			int i;
			for (i = 0; i < errors->length; ++i) {
				error_t *error = errors->items[i];
				fprintf(stderr, "%s:%d:%d: error #%d: %s\n", error->file_name,
						(int)error->line_number, (int)error->column, error->code,
						error->message);
				fprintf(stderr, "%s\n", error->line);
				if (error->column != 0) {
					int j;
					for (j = error->column; j > 0; --j) {
						fprintf(stderr, ".");
					}
					fprintf(stderr, "^\n");
				} else {
					fprintf(stderr, "\n");
				}
			}
			remove(scas_runtime.output_file);
		}
		if (warnings->length != 0) {
			int i;
			for (i = 0; i < errors->length; ++i) {
				warning_t *warning = warnings->items[i];
				fprintf(stderr, "%s:%d:%d: warning #%d: %s\n", warning->file_name,
						(int)warning->line_number, (int)warning->column, warning->code,
						get_warning_string(warning));
				fprintf(stderr, "%s\n", warning->line);
				if (warning->column != 0) {
					int j;
					for (j = warning->column; j > 0; --j) {
						fprintf(stderr, ".");
					}
					fprintf(stderr, "^\n");
				}
			}
		}
		ret = errors->length;
	}
	else {
		simple_log(SL_ERROR, "scas aborted\n");
		ret = -1;
	}
	scas_log(L_DEBUG, "Exiting with status code %d, cleaning up", ret);
	list_free(include_path);
	for (int i = 0; i < objects->length; i++) {
		object_free(objects->items[i]);
	}
	list_free(objects);
	list_free(errors);
	list_free(warnings);
	free(scas_runtime.output_file);
	free(scas_runtime.include_path);
	free_flat_list(scas_runtime.macros);
	instruction_set_free(set);
	*buffer = output;
	simple_log(SL_ERROR, "Assembler output: \"%s\"\n", output);
	return ret;
}

static toolchain_conversion_t conversions[] = {
	{
		.source_ext = ".asm",
		.target_ext = ".o"
	},
	{
		.source_ext = ".o",
		.target_ext = ".rom"
	}
};

list_t *get_conversions() {
	list_t *_conversions = create_list();
	list_add(_conversions, &conversions[0]);
	list_add(_conversions, &conversions[1]);
	return _conversions;
}

bool supports(const char *feature) {
	return !strcmp(feature, "Toolchain") || !strcmp(feature, "Assembler");
}

plugin_version_t version = {
	.major = 2,
	.minor = 1,
	.patch = 0,
	.str = "2.1.0"
};

toolchain_plugin_t toolchain = {
	.get_conversions = get_conversions,
	.convert = convert
};

plugin_t plugin = {
	.name = "SirCmpwn's Assembler",
	.supports = supports,
	.version = &version,
	.toolchain = &toolchain
};

PLUGIN_EXPORT void init() {

}

PLUGIN_EXPORT plugin_t *get_interface() {
	return &plugin;
}

PLUGIN_EXPORT void cleanup() {

}

int v = L_DEBUG;
int indent = 0;

void scas_logv(int verbosity, const char *format, va_list args) {
	if (verbosity > v)
		return;
	if (verbosity == L_DEBUG || verbosity == L_INFO) {
		if (out_len + indent >= out_cap) {
			out_cap += 512;
			output = realloc(output, out_cap);
			if (output == NULL) {
				exit(1);
			}
		}
		for (int i = 0; i < indent; ++i) {
			output[out_len++] = '\t';
			output[out_len] = 0;
		}
	}
	// Max 1KB per message
	char buffer[1024];
	int l = vsnprintf(buffer, 1024, format, args);
	if (l < 0) {
		simple_log(SL_ERROR, "Error appending to scas log\n");
	}
	else {
		if (l >= 1024) {
			simple_log(SL_WARN, "scas log message too big; dropped\n");
		}
		if (out_len + l + 2 >= out_cap) {
			out_cap += l + 2;
			output = realloc(output, out_cap);
			if (output == NULL) {
				exit(1);
			}
		}
		strcpy(output + out_len, buffer);
		out_len += l;
		strcpy(output + out_len, "\n");
		out_len++;
	}
}

void scas_log(int verbosity, char *format, ...) {
	va_list v;
	va_start(v, format);
	scas_logv(verbosity, format, v);
	va_end(v);
}

void scas_abort(char *format, ...) {
	va_list v;
	va_start(v, format);
	scas_logv(L_ERROR, format, v);
	va_end(v);
	longjmp(env, 1);
}

void init_log(int verbosity) {
	v = verbosity;
}

void set_verbosity(int verbosity) {
	if (verbosity > 1)
		verbosity--;
	simple_log(SL_DEBUG, "[scas] verbosity is at %d\n", verbosity);
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
