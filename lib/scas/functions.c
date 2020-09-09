#include "functions.h"
#include "objects.h"
#include "list.h"
#include "hashtable.h"
#include "expression.h"
#include "log.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#endif


unsigned int hash(void *data) {
	unsigned int hash = 5381;
	char *str = data;
	int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

symbol_t *get_symbol_by_name(area_t *area, const char *name) {
	int i;
	for (i = 0; i < area->symbols->length; ++i) {
		symbol_t *sym = area->symbols->items[i];
		if (strcasecmp(sym->name, name) == 0) {
			return sym;
		}
	}
	return NULL;
}

function_metadata_t *get_function_for_address(area_t *area, list_t *functions, uint32_t address) {
	int i;
	for (i = 0; i < functions->length; ++i) {
		function_metadata_t *func = functions->items[i];
		if (func->area == area && func->start_address <= address && func->end_address >= address) {
			return func;
		}
	}
	return NULL;
}

function_metadata_t *get_function_by_name(list_t *functions, const char *name) {
	int i;
	for (i = 0; i < functions->length; ++i) {
		function_metadata_t *func = functions->items[i];
		if (strcasecmp(func->name, name) == 0) {
			return func;
		}
	}
	return NULL;
}

void mark_dependencies_precious(function_metadata_t *parent, list_t *functions, object_t *object);

void mark_precious(list_t *functions, late_immediate_t *imm, object_t *object, int recurse) {
	int j;
	for (j = 0; j < imm->expression->tokens->length; ++j) {
		expression_token_t *tok = imm->expression->tokens->items[j];
		if (tok->type == SYMBOL) {
			function_metadata_t *func = get_function_by_name(functions, tok->symbol);
			if (func) {
				if (!func->precious) {
					scas_log(L_DEBUG, "Marked %s as precious", func->name);
					func->precious = 1;
					if (recurse) {
						mark_dependencies_precious(func, functions, object);
					}
				}
			}
		}
	}
}

void mark_dependencies_precious(function_metadata_t *parent, list_t *functions, object_t *object) {
	scas_log(L_DEBUG, "Marking dependencies of %s as precious", parent->name);
	for (int i = 0; i < object->areas->length; ++i) {
		area_t *area = object->areas->items[i];
		for (int j = 0; j < area->late_immediates->length; ++j) {
			late_immediate_t *imm = area->late_immediates->items[j];
			if (imm->base_address >= parent->start_address && imm->base_address <= parent->end_address) {
				mark_precious(functions, imm, object, 1);
			}
		}
	}
}

int compare_functions(const void *a, const void *b) {
	const function_metadata_t *func_a = a;
	const function_metadata_t *func_b = b;
	return func_a->start_address - func_b->start_address;
}

void remove_unused_functions(object_t *object) {
	scas_log(L_DEBUG, "Optimizing out unused functions for object");
	list_t *functions = create_list();
	int i;
	for (i = 0; i < object->areas->length; ++i) {
		area_t *area = object->areas->items[i];
		metadata_t *meta = get_area_metadata(area, "scas.functions");
		if (meta) {
			list_t *decoded = decode_function_metadata(area, meta->value);
			list_cat(functions, decoded);
			list_free(decoded);
		}
	}
	for (i = 0; i < functions->length; ++i) {
		function_metadata_t *meta = functions->items[i];
		meta->precious = 0;
		symbol_t *start = get_symbol_by_name(meta->area, meta->start_symbol);
		symbol_t *end = get_symbol_by_name(meta->area, meta->end_symbol);
		if (!start || !end) {
			scas_log(L_ERROR, "Warning: function %s has unknown start and end symbols", meta->name);
			function_metadata_t *func = functions->items[i];
			free(func->name);
			free(func->start_symbol);
			free(func->end_symbol);
			free(func);
			list_del(functions, i);
			--i;
		} else {
			scas_log(L_DEBUG, "Function %s is located at %08X-%08X", meta->name, start->value, end->value);
			meta->start_address = start->value;
			meta->end_address = end->value;
		}
	}
	scas_log(L_DEBUG, "Found %d functions, building dependency map", functions->length);
	for (i = 0; i < object->areas->length; ++i) {
		area_t *a = object->areas->items[i];
		int j;
		for (j = 0; j < a->late_immediates->length; ++j) {
			late_immediate_t *imm = a->late_immediates->items[j];
			function_metadata_t *meta = get_function_for_address(a, functions, imm->address);
			if (meta == NULL) { // Precious code
				mark_precious(functions, imm, object, 0);
			}
		}
	}
	for (i = 0; i < functions->length; ++i) {
		function_metadata_t *func = functions->items[i];
		if (func->precious) {
			mark_dependencies_precious(func, functions, object);
		}
	}
	for (i = 0; i < functions->length; ++i) {
		function_metadata_t *func = functions->items[i];
		if (!func->precious) {
			scas_log(L_DEBUG, "Removing unused function %s (at %08X - %08X)", func->name, func->start_address, func->end_address);
			int length = func->end_address - func->start_address;
			delete_from_area(func->area, func->start_address, length);
			int k;
			for (k = 0; k < func->area->symbols->length; ++k) {
				symbol_t *sym = func->area->symbols->items[k];
				if (sym->type == SYMBOL_LABEL) {
					if (sym->value >= func->start_address && sym->value < func->end_address) {
						list_del(func->area->symbols, k);
						k -= 1;
					} else if (sym->value >= func->end_address) {
						sym->value -= length;
					}
				}
			}
			int j;
			for (j = 0; j < func->area->late_immediates->length; ++j) {
				late_immediate_t *_imm = func->area->late_immediates->items[j];
				if (_imm->address >= func->start_address && _imm->address < func->end_address) {
					list_del(func->area->late_immediates, j--);
				} else if (_imm->address >= func->end_address) {
					_imm->base_address -= length;
					_imm->instruction_address -= length;
					_imm->address -= length;
				}
			}
			for (j = 0; j < func->area->source_map->length; ++j) {
				source_map_t *map = func->area->source_map->items[j];
				for (k = 0; k < map->entries->length; ++k) {
					source_map_entry_t *entry = map->entries->items[k];
					if (entry->address >= func->start_address && entry->address < func->end_address) {
						free(entry->source_code);
						free(entry);
						list_del(map->entries, k--);
					} else if (entry->address >= func->end_address) {
						entry->address -= length;
					}
				}
			}
			for (k = 0; k < functions->length; ++k) {
				function_metadata_t *_func = functions->items[k];
				if (_func->start_address >= func->end_address) {
					_func->start_address -= length;
					_func->end_address -= length;
				}
			}
		}
	}
	for (int i = 0; i < functions->length; i++) {
		function_metadata_t *func = functions->items[i];
		free(func->name);
		free(func->start_symbol);
		free(func->end_symbol);
		free(func);
	}
	list_free(functions);
}

list_t *decode_function_metadata(area_t *area, char *value) {
	uint32_t total;
	list_t *result = create_list();
	total = *(uint32_t *)value;
	value += sizeof(uint32_t);

	if (total > 10000) {
		scas_log(L_ERROR, "More than 10,000 functions detected. This is probably an internal error.");
		list_free(result);
		return NULL;
	}
	scas_log(L_DEBUG, "Decoding metadata for %d functions", (int)total);
	for (uint32_t i = 0; i < total; ++i) {
		uint32_t len;
		function_metadata_t *meta = calloc(1, sizeof(function_metadata_t));
		meta->area = area;
		len = *(uint32_t *)value;
		value += sizeof(uint32_t);
		meta->name = malloc(len + 1);
		strcpy(meta->name, value);
		value += len + 1;

		len = *(uint32_t *)value;
		value += sizeof(uint32_t);
		meta->start_symbol = malloc(len + 1);
		strcpy(meta->start_symbol, value);
		value += len + 1;

		len = *(uint32_t *)value;
		value += sizeof(uint32_t);
		meta->end_symbol = malloc(len + 1);
		strcpy(meta->end_symbol, value);
		value += len + 1;

		list_add(result, meta);
	}
	return result;
}

char *encode_function_metadata(list_t *metadata, uint64_t *value_length) {
	uint32_t len = sizeof(uint32_t);
	int i;
	for (i = 0; i < metadata->length; ++i) {
		function_metadata_t *meta = metadata->items[i];
		len += strlen(meta->name) + 1 + sizeof(uint32_t);
		len += strlen(meta->start_symbol) + 1 + sizeof(uint32_t);
		len += strlen(meta->end_symbol) + 1 + sizeof(uint32_t);
	}
	char *result = malloc(len);
	scas_log(L_DEBUG, "Allocated %d bytes for function metadata", len);
	*value_length = (uint32_t)len;
	*(uint32_t *)result = (uint32_t)metadata->length;
	int ptr = sizeof(uint32_t);
	for (i = 0; i < metadata->length; ++i) {
		function_metadata_t *meta = metadata->items[i];

		*(uint32_t *)(result + ptr) = (uint32_t)strlen(meta->name);
		ptr += sizeof(uint32_t);
		strcpy(result + ptr, meta->name);
		ptr += strlen(meta->name) + 1;

		*(uint32_t *)(result + ptr) = (uint32_t)strlen(meta->start_symbol);
		ptr += sizeof(uint32_t);
		strcpy(result + ptr, meta->start_symbol);
		ptr += strlen(meta->start_symbol) + 1;
		
		*(uint32_t *)(result + ptr) = (uint32_t)strlen(meta->end_symbol);
		ptr += sizeof(uint32_t);
		strcpy(result + ptr, meta->end_symbol);
		ptr += strlen(meta->end_symbol) + 1;
	}
	return result;
}
