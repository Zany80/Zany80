#include "objects.h"
#include "log.h"
#include "functions.h"
#include "readline.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define SCASOBJ_VERSION 2

object_t *create_object() {
	object_t *o = malloc(sizeof(object_t));
	o->areas = create_list();
	o->exports = create_list();
	return o;
}

void object_free(object_t *o) {
	list_free(o->areas);
}

area_t *create_area(const char *name) {
	area_t *a = malloc(sizeof(area_t));
	a->name = malloc(strlen(name) + 1);
	strcpy(a->name, name);
	a->late_immediates = create_list();
	a->symbols = create_list();
	a->source_map = create_list();
	a->metadata = create_list();
	a->final_address = 0;
	a->data_length = 0;
	a->data_capacity = 1024;
	a->data = malloc(a->data_capacity);
	return a;
}

metadata_t *get_area_metadata(area_t *area, const char *key) {
	int i;
	for (i = 0; i < area->metadata->length; ++i) {
		metadata_t *meta = area->metadata->items[i];
		if (strcmp(meta->key, key) == 0) {
			return meta;
		}
	}
	return NULL;
}

void set_area_metadata(area_t *area, const char *key, char *value, uint64_t value_length) {
	int i;
	for (i = 0; i < area->metadata->length; ++i) {
		metadata_t *meta = area->metadata->items[i];
		if (strcmp(meta->key, key) == 0) {
			list_del(area->metadata, i);
			break;
		}
	}
	metadata_t *newmeta = malloc(sizeof(metadata_t));
	newmeta->key = malloc(strlen(key) + 1);
	strcpy(newmeta->key, key);
	newmeta->value_length = value_length;
	newmeta->value = value;
	scas_log(L_DEBUG, "Set area metadata '%s' to new value with length %d", newmeta->key, newmeta->value_length);
	list_add(area->metadata, newmeta);
}

void append_to_area(area_t *area, uint8_t *data, size_t length) {
	while ((area->data_capacity - area->data_length) < length) {
		/* Expand capacity */
		area->data = realloc(area->data, area->data_capacity + 1024);
		area->data_capacity += 1024;
	}
	memcpy(area->data + area->data_length, data, length);
	area->data_length += length;
	scas_log(L_DEBUG, "Added %d bytes to area '%s' (now %d bytes total)", length, area->name, area->data_length);
}

void insert_in_area(area_t *area, uint8_t *data, size_t length, size_t index) {
	while (area->data_capacity < length + area->data_length) {
		/* Expand capacity */
		area->data = realloc(area->data, area->data_capacity + 1024);
		area->data_capacity += 1024;
	}
	memmove(area->data + index + length, area->data + index, area->data_length - index);
	memcpy(area->data + index, data, length);
	area->data_length += length;
	scas_log(L_DEBUG, "Inserted %d bytes in area '%s' (now %d bytes total)", length, area->name, area->data_length);
}

void delete_from_area(area_t *area, size_t index, size_t length) {
	scas_log(L_DEBUG, "Removing %d bytes at %08X from area '%s'", length, index, area->name);
	memmove(area->data + index, area->data + index + length, area->data_length - (index + length));
	area->data_length -= length;
}

void write_area(FILE *f, area_t *a) {
	uint32_t len;
	uint64_t len64;
	int i;
	fprintf(f, "%s", a->name); fputc(0, f);
	/* Symbols */
	fwrite(&a->symbols->length, sizeof(uint32_t), 1, f);
	for (i = 0; i < a->symbols->length; ++i) {
		symbol_t *sym = a->symbols->items[i];
		fputc(sym->exported, f);
		len = strlen(sym->name);
		fwrite(&len, sizeof(uint32_t), 1, f);
		fprintf(f, "%s", sym->name);
		fwrite(&sym->value, sizeof(uint64_t), 1, f);
		fwrite(&sym->defined_address, sizeof(uint64_t), 1, f);
	}
	/* Imports (TODO) */
	/* Expressions */
	fwrite(&a->late_immediates->length, sizeof(uint32_t), 1, f);
	for (i = 0; i < a->late_immediates->length; ++i) {
		late_immediate_t *imm = a->late_immediates->items[i];
		fputc(imm->type, f);
		fputc(imm->width, f);
		fwrite(&imm->instruction_address, sizeof(uint64_t), 1, f);
		fwrite(&imm->base_address, sizeof(uint64_t), 1, f);
		fwrite(&imm->address, sizeof(uint64_t), 1, f);
		fwrite_tokens(f, imm->expression);
	}
	/* Machine code */
	fwrite(&a->data_length, sizeof(uint64_t), 1, f);
	fwrite(a->data, sizeof(uint8_t), a->data_length, f);
	/* Metadata */
	fwrite(&a->metadata->length, sizeof(uint64_t), 1, f);
	for (i = 0; i < a->metadata->length; ++i) {
		metadata_t *meta = a->metadata->items[i];
		fputc((uint8_t)strlen(meta->key), f);
		fwrite(meta->key, sizeof(char), strlen(meta->key), f);
		fwrite(&meta->value_length, sizeof(uint64_t), 1, f);
		fwrite(meta->value, sizeof(char), meta->value_length, f);
	}
	/* Source map */
	fwrite(&a->source_map->length, sizeof(uint64_t), 1, f);
	for (i = 0; i < a->source_map->length; ++i) {
		source_map_t *map = a->source_map->items[i];
		fwrite(map->file_name, sizeof(char), strlen(map->file_name), f);
		fputc(0, f);
		len64 = map->entries->length;
		fwrite(&len64, sizeof(uint64_t), 1, f);
		int j;
		for (j = 0; j < map->entries->length; ++j) {
			source_map_entry_t *entry = map->entries->items[j];
			fwrite(&entry->line_number, sizeof(uint64_t), 1, f);
			fwrite(&entry->address, sizeof(uint64_t), 1, f);
			fwrite(&entry->length, sizeof(uint64_t), 1, f);
			fwrite(entry->source_code, sizeof(char), strlen(entry->source_code), f);
			fputc(0, f);
		}
	}
}

void fwriteobj(FILE *f, object_t *o) {
	/* Header */
	fprintf(f, "SCASOBJ");
	fputc(SCASOBJ_VERSION, f);
	/* Areas */
	uint32_t a_len = o->areas->length;
	fwrite(&a_len, sizeof(uint32_t), 1, f);
	int i;
	for (i = 0; i < o->areas->length; ++i) {
		area_t *a = o->areas->items[i];
		write_area(f, a);
	}
	fflush(f);
}

area_t *read_area(FILE *f) {
	char *name = read_line(f);
	area_t *area = create_area(name);
	scas_log(L_DEBUG, "Reading area '%s' from file", name);
	uint32_t symbols, immediates;
	fread(&symbols, sizeof(uint32_t), 1, f);
	uint32_t len;
	int i;
	for (i = 0; i < symbols; ++i) {
		symbol_t *sym = malloc(sizeof(symbol_t));
		sym->exported = fgetc(f);
		fread(&len, sizeof(uint32_t), 1, f);
		sym->name = calloc(len + 1, sizeof(char));
		fread(sym->name, sizeof(char), len, f);
		fread(&sym->value, sizeof(uint64_t), 1, f);
		fread(&sym->defined_address, sizeof(uint64_t), 1, f);
		sym->type = SYMBOL_LABEL;
		list_add(area->symbols, sym);
		scas_log(L_DEBUG, "Read symbol '%s' with value 0x%08X%08X", sym->name, (uint32_t)(sym->value >> 32), (uint32_t)sym->value);
	}
	/* TODO: Imports */
	fread(&immediates, sizeof(uint32_t), 1, f);
	for (i = 0; i < immediates; ++i) {
		late_immediate_t *imm = malloc(sizeof(late_immediate_t));
		imm->type = fgetc(f);
		imm->width = fgetc(f);
		fread(&imm->instruction_address, sizeof(uint64_t), 1, f);
		fread(&imm->base_address, sizeof(uint64_t), 1, f);
		fread(&imm->address, sizeof(uint64_t), 1, f);
		imm->expression = fread_tokenized_expression(f);
		list_add(area->late_immediates, imm);
		scas_log(L_DEBUG, "Read immediate value at 0x%08X (width: %d)", imm->address, imm->width);
	}
	fread(&area->data_length, sizeof(uint64_t), 1, f);
	area->data_capacity = area->data_length;
	free(area->data);
	area->data = malloc((int)area->data_length);
	fread(area->data, sizeof(uint8_t), (int)area->data_length, f);
	scas_log(L_DEBUG, "Read %d bytes of machine code", area->data_length);

	uint64_t meta_length, meta_key;
	fread(&meta_length, sizeof(uint64_t), 1, f);
	scas_log(L_DEBUG, "Reading %d metadata entries", meta_length);
	for (i = 0; i < (int)meta_length; ++i) {
		metadata_t *meta = malloc(sizeof(metadata_t));
		meta_key = fgetc(f);
		meta->key = malloc(meta_key);
		fread(meta->key, sizeof(char), meta_key, f);
		fread(&meta->value_length, sizeof(uint64_t), 1, f);
		meta->value = malloc(meta->value_length);
		fread(meta->value, sizeof(char), meta->value_length, f);
		list_add(area->metadata, meta);
		scas_log(L_DEBUG, "Read metadata %s with value length %d", meta->key, meta->value_length);
	}

	uint64_t fileno, lineno;
	fread(&fileno, sizeof(uint64_t), 1, f);
	for (i = 0; i < (int)fileno; ++i) {
		source_map_t *map = malloc(sizeof(source_map_t));
		map->file_name = read_line(f);
		map->entries = create_list();
		fread(&lineno, sizeof(uint64_t), 1, f);
		scas_log(L_DEBUG, "Reading source map for '%s', %d entries", map->file_name, lineno);
		int j;
		for (j = 0; j < (int)lineno; ++j) {
			source_map_entry_t *entry = malloc(sizeof(source_map_entry_t));
			fread(&entry->line_number, sizeof(uint64_t), 1, f);
			fread(&entry->address, sizeof(uint64_t), 1, f);
			fread(&entry->length, sizeof(uint64_t), 1, f);
			entry->source_code = read_line(f);
			list_add(map->entries, entry);
			scas_log(L_DEBUG, "Read entry at 0x%08X%08X (line %d): %s", (uint32_t)(entry->address >> 32), (uint32_t)entry->address, entry->line_number, entry->source_code);
		}
		list_add(area->source_map, map);
	}
	return area;
}

object_t *freadobj(FILE *f, const char *name) {
	object_t  *o = create_object();
	char magic[7];
	int len = fread(magic, sizeof(char), 7, f);
	if (len != 7 || strncmp("SCASOBJ", magic, 7) != 0) {
		scas_abort("'%s' is not a valid object file.", name);
	}
	int ver = fgetc(f);
	if (ver != SCASOBJ_VERSION) {
		scas_abort("'%s' was built with an incompatible version of scas.", name);
	}
	uint32_t area_count;
	fread(&area_count, sizeof(uint32_t), 1, f);
	int i;
	for (i = 0; i < area_count; ++i) {
		list_add(o->areas, read_area(f));
	}
	return o;
}

source_map_t *create_source_map(area_t *area, const char *file_name) {
	source_map_t *map = malloc(sizeof(source_map_t));
	map->file_name = malloc(strlen(file_name) + 1);
	strcpy(map->file_name, file_name);
	map->entries = create_list();
	list_add(area->source_map, map);
	return map;
}

void add_source_map(source_map_t *map, int line_number, const char *line, uint64_t address, uint64_t length) {
	source_map_entry_t *entry = malloc(sizeof(source_map_entry_t));
	entry->line_number = line_number;
	entry->address = address;
	entry->length = length;
	entry->source_code = malloc(strlen(line) + 1);
	strcpy(entry->source_code, line);
	list_add(map->entries, entry);
}
