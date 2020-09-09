#include "merge.h"
#include "linker.h"
#include "objects.h"
#include "errors.h"
#include "functions.h"
#include "list.h"
#include "log.h"
#include "stringop.h"
#include <stdint.h>
#include <stdlib.h>
#ifndef _WIN32
#include <strings.h>
#endif

area_t *get_area_by_name(object_t *object, char *name) {
	int i;
	for (i = 0; i < object->areas->length; ++i) {
		area_t *a = object->areas->items[i];
		if (strcasecmp(a->name, name) == 0) {
			return a;
		}
	}
	return NULL;
}

void relocate_area(area_t *area, uint64_t address, bool immediates) {
	int i;
	scas_log(L_DEBUG, "Assigning final address %08X to area %s", address, area->name);
	area->final_address = address;
	for (i = 0; i < area->symbols->length; ++i) {
		symbol_t *sym = area->symbols->items[i];
		if (sym->type != SYMBOL_LABEL) {
			continue;
		}
		sym->defined_address += address;
		sym->value += address;
	}
	for (i = 0; immediates && i < area->late_immediates->length; ++i) {
		late_immediate_t *imm = area->late_immediates->items[i];
		imm->address += address;
		imm->instruction_address += address;
		imm->base_address += address;
	}
	for (i = 0; immediates && i < area->source_map->length; ++i) {
		source_map_t *map = area->source_map->items[i];
		int j;
		for (j = 0; j < map->entries->length; ++j) {
			source_map_entry_t *entry = map->entries->items[j];
			entry->address += address;
		}
	}
}

bool merge_areas(object_t *merged, object_t *source) {
	for (int i = 0; i < source->areas->length; ++i) {
		area_t *source_area = source->areas->items[i];
		area_t *merged_area = get_area_by_name(merged, source_area->name);
		if (merged_area == NULL) {
			merged_area = create_area(source_area->name);
			list_add(merged->areas, merged_area);
		}
		uint64_t new_address = merged_area->data_length;
		relocate_area(source_area, new_address, true);

		append_to_area(merged_area, source_area->data, source_area->data_length);
		list_cat(merged_area->symbols, source_area->symbols);
		list_cat(merged_area->late_immediates, source_area->late_immediates);
		// NOTE: I know this is not very good SoC
		metadata_t *new_functions = get_area_metadata(source_area, "scas.functions");
		metadata_t *old_functions = get_area_metadata(merged_area, "scas.functions");
		if (new_functions) {
			list_t *decoded = decode_function_metadata(source_area, new_functions->value);
			if (!decoded) {
				return false;
			}
			list_t *merged;
			if (old_functions) {
				merged = decode_function_metadata(source_area, old_functions->value);
			} else {
				merged = create_list();
			}
			if (!merged) {
				list_free(decoded);
				return false;
			}
			list_cat(merged, decoded);
			list_free(decoded);
			uint64_t len;
			char *merged_metadata = encode_function_metadata(merged, &len);
			for (int i = 0; i < merged->length; i++) {
				function_metadata_t *func = merged->items[i];
				free(func->name);
				free(func->start_symbol);
				free(func->end_symbol);
			}
			free_flat_list(merged);
			set_area_metadata(merged_area, "scas.functions", merged_metadata, len);
			free(merged_metadata);
		}
		list_cat(merged_area->source_map, source_area->source_map);
	}
	return true;
}

object_t *merge_objects(list_t *objects) {
	scas_log(L_INFO, "Merging %d objects into one", objects->length);
	object_t *merged = create_object();
	merged->merged = true;
	int i;
	for (i = 0; i < objects->length; ++i) {
		object_t *o = objects->items[i];
		scas_log(L_DEBUG, "Merging object %d", i);
		if (!merge_areas(merged, o)) {
			object_free(merged);
			return NULL;
		}
	}
	return merged;
}
