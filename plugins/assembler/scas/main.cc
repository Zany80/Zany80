#include <Core/Args.h>

#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#include "scas.h"
#include <scas_version.h>

#include <assembler.h>
#include <runtime.h>
#include <log.h>
#include <bin.h>
#include <generated.h>
#include <instructions.h>
#include <merge.h>
#include <errors.h>

list_t *split_include_path() {
	list_t *list = create_list();
	int i, j;
	for (i = 0, j = 0; scas_runtime.include_path[i]; ++i) {
		if (scas_runtime.include_path[i] == ':' || scas_runtime.include_path[i] == ';') {
			char *s = new char[i - j + 1];
			strncpy(s, scas_runtime.include_path + j, i - j);
			s[i - j] = 0;
			j = i + 1;
			list_add(list, s);
		}
	}
	char *s = new char[i - j + 1];
	strncpy(s, scas_runtime.include_path + j, i - j);
	s[i - j] = 0;
	list_add(list, s);
	return list;
}

bool scas::supports(String type) {
	return (Array<String>{
		"Invocation", "Toolchain"
	}).FindIndexLinear(type) != InvalidIndex;
}

struct runtime scas_runtime;
StringBuilder *out;

Map<String, Array<String>> scas::supportedTransforms() {
	return Map<String, Array<String>> {
		{ ".asm", {
			".o",
			".rom"
		}},
		{ ".o", {
			".rom"
		}}
	};
}

static jmp_buf env;

int scas::transform(Array<String> sources, String destination, StringBuilder *output) {
	out = output;
	out->Append("scas version " PROJECT_VERSION " loaded!\n");
	StringBuilder sDst(destination);
	if (sources.Size() == 0) {
		output->Append("Error: no source files specified!\n");
		return 1;
	}
	if (sDst.FindLastOf(0, EndOfString, ".") == InvalidIndex) {
		output->Append("Error: destination file does not have an extension!\n");
		return 2;
	}
	jobs_t job;
	String extension = sDst.GetSubString(sDst.FindLastOf(0, EndOfString, "."), EndOfString);
	if (extension == ".rom") {
		job = LINK;
	}
	else if (extension == ".o") {
		job = ASSEMBLE;
	}
	else {
		output->Append("Error: output has to be object file or ROM!\n");
		return 3;
	}
	scas_runtime.arch = "z80";
	scas_runtime.jobs = job;
	scas_runtime.macros = create_list();
	scas_runtime.output_type = EXECUTABLE;
	scas_runtime.input_files = create_list();
	scas_runtime.output_file = new char[destination.Length() + 1];
	strcpy(scas_runtime.output_file, destination.AsCStr());
	scas_runtime.listing_file = NULL;
	scas_runtime.symbol_file = NULL;
	scas_runtime.include_path = new char[include_dirs.Length() + 1];
	strcpy(scas_runtime.include_path, include_dirs.AsCStr());
	scas_runtime.linker_script = NULL;
	scas_runtime.options.explicit_export = true;
	scas_runtime.options.explicit_import = true;
	scas_runtime.options.auto_relocation = false;
	scas_runtime.options.remove_unused_functions = false;
	scas_runtime.options.output_format = output_bin;
	scas_runtime.options.origin = 0;
	for (String s : sources) {
		list_add(scas_runtime.input_files, (char*)s.AsCStr());
	}
	instruction_set_t *z80 = load_instruction_set_s(z80_tab);
	scas_log(L_INFO, "Loaded z80 instruction set");
	
	list_t *include_path = split_include_path();
	list_t *errors = create_list();
	list_t *warnings = create_list();
	
	list_t *objects = create_list();
	
	if (setjmp(env)) {
		return -1;
	}
	
	int i;
	for (i = 0; i < scas_runtime.input_files->length; ++i) {
		out->AppendFormat(512, "Assembling input file: '%s'\n", (char*)scas_runtime.input_files->items[i]);
		indent_log();
		FILE *f = fopen((const char *)scas_runtime.input_files->items[i], "r");
		if (!f) {
			out->AppendFormat(512, "Unable to open '%s' for assembly.\n", (char*)scas_runtime.input_files->items[i]);
			return 4;
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
			o = freadobj(f, (const char *)scas_runtime.input_files->items[i]);
		} 
		else {
			assembler_settings_t settings = {
				.set = z80,
				.include_path = include_path,
				.errors = errors,
				.warnings = warnings,
				.macros = scas_runtime.macros,
			};
			o = assemble(f, (const char *)scas_runtime.input_files->items[i], &settings);
			fclose(f);
			scas_log(L_INFO, "Assembler returned %d errors, %d warnings for '%s'",
					errors->length, warnings->length, scas_runtime.input_files->items[i]);
			if (errors->length > 0) {
				scas_log(L_ERROR, "Error assembling!");
				for (int i = 0; i < errors->length; i++) {
					scas_error_t *error = ((scas_error_t**)errors->items)[i];
					scas_log(L_ERROR, "Error on line %u: %s", error->line_number, error->message);
				}
				return -1;
			}
		}
		list_add(objects, o);
		deindent_log();
	}
	scas_log(L_DEBUG, "Opening output file for writing: %s", scas_runtime.output_file);
	FILE *out = fopen(scas_runtime.output_file, "w+");
	if (!out) {
		scas_abort("Unable to open '%s' for output.", scas_runtime.output_file);
	}
	if ((scas_runtime.jobs & LINK) == LINK) {
		scas_log(L_INFO, "Linking...");
		linker_settings_t settings = {
			.automatic_relocation = scas_runtime.options.auto_relocation,
			.merge_only = (scas_runtime.jobs & MERGE) == MERGE,
			.errors = errors,
			.warnings = warnings,
			.write_output = scas_runtime.options.output_format
		};
		if (settings.merge_only) {
			object_t *merged = merge_objects(objects);
			fwriteobj(out, merged);
		} else {
			link_objects(out, objects, &settings);
			fflush(out);
			fclose(out);
		}
		scas_log(L_INFO, "Linker returned %d errors, %d warnings", errors->length, warnings->length);
		if (errors->length > 0) {
			scas_log(L_ERROR, "Error linking!");
			for (int i = 0; i < errors->length; i++) {
				scas_error_t *error = ((scas_error_t**)errors->items)[i];
				scas_log(L_ERROR, "Error on line %u: %s", error->line_number, error->message);
			}
			return -1;
		}
	} else {
		scas_log(L_INFO, "Skipping linking - writing to object file");
		object_t *merged = merge_objects(objects);
		fwriteobj(out, merged);
		fflush(out);
		fclose(out);
	}
	scas_log(-1, "Assembled successfully!\n");
	return 0;
}

void scas_abort(const char *format, ...) {
	va_list args;
	va_start(args, format);
	scas_log(L_ERROR, format, args);
	va_end(args);
	longjmp(env, 1);
}

void scas::addIncludeDirectory(String dir) {
	include_dirs.AppendFormat(512, ":%s", dir.AsCStr());
}

String scas::getChain() {
	return "Official";
}

#ifndef ORYOL_EMSCRIPTEN

extern "C" Plugin *make() {
	return new scas;
}

#endif
