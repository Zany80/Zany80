#include "linker.h"
#include "objects.h"
#include "errors.h"
#include "list.h"
#include "expression.h"
#include "string.h"
#include "instructions.h"
#include "functions.h"
#include "merge.h"
#include "runtime.h"
#include "log.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#ifndef _WIN32
#include <strings.h>
#endif


/*
 * Notes on how this could be improved:
 * A sorted list_t would make a lot of these lookups faster
 * A hashtable could also be used to handle dupes and have fast lookup
 */

symbol_t *find_symbol(list_t *symbols, char *name) {
	int i;
	for (i = 0; i < symbols->length; ++i) {
		symbol_t *sym = symbols->items[i];
		if (strcasecmp(sym->name, name) == 0) {
			return sym;
		}
	}
	return NULL;
}

void resolve_immediate_values(list_t *symbols, area_t *area, list_t *errors) {
	scas_log(L_DEBUG, "Resolving immediate values for area '%s' at %08X", area->name, area->final_address);
	indent_log();
	int i;
	for (i = 0; i < area->late_immediates->length; ++i) {
		late_immediate_t *imm = area->late_immediates->items[i];
		imm->instruction_address += area->final_address;
		imm->base_address += area->final_address;
		/* Temporarily add $ to symbol list */
		symbol_t sym_pc = {
			.type = SYMBOL_LABEL,
			.value = imm->instruction_address,
			.name = "$"
		};
		list_add(symbols, &sym_pc);
		int error;
		char *symbol;
		uint64_t result = evaluate_expression(imm->expression, symbols, &error, &symbol);

		list_del(symbols, symbols->length - 1); // Remove $
		if (error == EXPRESSION_BAD_SYMBOL) {
			scas_log(L_ERROR, "Unable to find symbol for expression");
			add_error_from_map(errors, ERROR_UNKNOWN_SYMBOL, area->source_map,
					imm->instruction_address, symbol);
			continue;
		} else if (error == EXPRESSION_BAD_SYNTAX) {
			add_error_from_map(errors, ERROR_INVALID_SYNTAX, area->source_map,
					imm->instruction_address);
			continue;
		} else {
			if (imm->type == IMM_TYPE_RELATIVE) {
				result = result - imm->base_address;
			}
			scas_log(L_DEBUG, "Immediate value result: 0x%08X (width %d, base address 0x%08X)", result, imm->width, imm->base_address);
			uint64_t mask = 1;
			int shift = imm->width;
			while (--shift) {
				mask <<= 1;
				mask |= 1;
			}
			if (imm->type == IMM_TYPE_RELATIVE) { // Signed
				if ((result & (mask >> 1)) != result) {
					if (!(result & (1 << imm->width)) || (result & ~mask) != ~mask) {
						add_error_from_map(errors, ERROR_VALUE_TRUNCATED,
								area->source_map, imm->instruction_address);
					}
				}
			} else {
				if ((result & mask) != result && ~result >> imm->width) {
					add_error_from_map(errors, ERROR_VALUE_TRUNCATED,
							area->source_map, imm->instruction_address);
				}
			}
			result = result & mask;
			for (size_t j = 0; j < imm->width / 8; ++j) {
				area->data[imm->address + j] |= (result & 0xFF);
				result >>= 8;
			}
		}
	}
	deindent_log();
}

/* z80 only (and possibly ez80) */
void auto_relocate_area(area_t *area, area_t *runtime) {
	scas_log(L_DEBUG, "Performing automatic relocation for %s", area->name);
	uint8_t rst0x8 = 0xCF;
	int i;
	for (i = 0; i < area->late_immediates->length; ++i) {
		late_immediate_t *imm = area->late_immediates->items[i];
		if (imm->type != IMM_TYPE_RELATIVE) {
			if (imm->base_address != imm->address) {
				/* Relocate this */
				scas_log(L_DEBUG, "Adding relocation instruction for immediate at 0x%08X (inserting at 0x%08X)", imm->address, imm->instruction_address);
				insert_in_area(area, &rst0x8, sizeof(uint8_t), imm->instruction_address);
				++imm->address;
				/* Move everything that comes after */
				int k;
				for (k = 0; k < area->symbols->length; ++k) {
					symbol_t *sym = area->symbols->items[k];
					if (sym->type == SYMBOL_LABEL && sym->value > imm->instruction_address) {
						++sym->value;
					}
				}
				int j;
				for (j = 0; j < area->late_immediates->length; ++j) {
					late_immediate_t *_imm = area->late_immediates->items[j];
					if (_imm->base_address > imm->base_address) {
						++_imm->base_address;
						++_imm->instruction_address;
						++_imm->address;
					}
				}
			} else {
				/* Relocate this */
				uint16_t value = (uint16_t)(imm->address + area->final_address);
				append_to_area(runtime, (uint8_t*)&value, sizeof(uint16_t));
			}
		}
	}
}

void gather_symbols(list_t *symbols, area_t *area, linker_settings_t *settings) {
	int i;
	for (i = 0; i < area->symbols->length; ++i) {
		symbol_t *sym = area->symbols->items[i];
		if (find_symbol(symbols, sym->name)) {
			add_error_from_map(settings->errors, ERROR_DUPLICATE_SYMBOL,
					area->source_map, sym->defined_address, sym->name);
		} else {
			list_add(symbols, sym);
		}
	}
}

void move_origin(list_t *symbols) {
	int i;
	for (i = 0; i < symbols->length; ++i) {
		symbol_t *sym = symbols->items[i];
		sym->value += scas_runtime.options.origin;
	}
}

void link_objects(FILE *output, list_t *objects, linker_settings_t *settings) {
	list_t *symbols = create_list(); // TODO: Use a hash table

	/* Create a new area for relocatable references */
	area_t *runtime = NULL;
	if (settings->automatic_relocation) {
		const char *sym_name = "__scas_relocatable_data";
		runtime = create_area("__scas_relocatable");
		symbol_t *sym = malloc(sizeof(symbol_t));
		sym->type = SYMBOL_LABEL;
		sym->name = strdup(sym_name);
		sym->value = 0;
		sym->defined_address = 0;
		sym->exported = 0;
		list_add(runtime->symbols, sym);
		object_t *o = create_object();
		list_add(o->areas, runtime);
		list_add(objects, o);
	}

	object_t *merged = merge_objects(objects);
	if (!merged) {
		list_free(symbols);
		return;
	}

	area_t *final = create_area("FINAL");

	runtime = get_area_by_name(merged, "__scas_relocatable");

	scas_log(L_INFO, "Assigning final address for all areas");
	if (scas_runtime.options.remove_unused_functions) {
		remove_unused_functions(merged);
	}
	uint64_t address = 0;
	for (int i = 0; i < merged->areas->length; ++i) {
		area_t *area = merged->areas->items[i];
		relocate_area(area, address, false);
		if (settings->automatic_relocation) {
			if (area == runtime) {
				uint16_t null_ptr = 0;
				append_to_area(area, (uint8_t *)&null_ptr, sizeof(uint16_t));
			} else {
				auto_relocate_area(area, runtime);
			}
		}
		address += area->data_length;
	}
	for (int i = 0; i < merged->areas->length; ++i) {
		area_t *area = merged->areas->items[i];
		gather_symbols(symbols, area, settings);
	}
	for (int i = 0; i < merged->areas->length; ++i) {
		area_t *area = merged->areas->items[i];
		scas_log(L_INFO, "Linking area %s", area->name);
		if (scas_runtime.options.origin) {
			move_origin(symbols);
		}
		resolve_immediate_values(symbols, area, settings->errors);
		scas_log(L_DEBUG, "Writing final linked area to output file");
		append_to_area(final, area->data, area->data_length);
	}
	settings->write_output(output, final->data, (int)final->data_length);
	scas_log(L_DEBUG, "Final binary written: %d bytes", ftell(output));

	if (scas_runtime.symbol_file) {
		scas_log(L_DEBUG, "Generating symbol file '%s'", scas_runtime.symbol_file);
		FILE *symfile = fopen(scas_runtime.symbol_file, "w");
		for (int i = 0; i < symbols->length; i++) {
			symbol_t *symbol = symbols->items[i];
			if (symbol->type == SYMBOL_LABEL && symbol->exported && !strchr(symbol->name, '@')) {
				fprintf(symfile, ".equ %s 0x%lX\n", symbol->name, symbol->value);
			}
		}
		fflush(symfile);
		fclose(symfile);
	}

	object_free(merged);
	area_free(final);
	list_free(symbols);
}
