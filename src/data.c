#include "SIMPLE/data.h"
#include "SIMPLE/API.h"
#include "SIMPLE/XML.h"
#include "SIMPLE/3rd-party/cfgpath.h"
#include "SIMPLE/3rd-party/stretchy_buffer.h"

#include <stdio.h>

typedef struct {
	char *name;
	char *path;
	char *version;
} data_descriptor_t;

typedef struct {
	size_t known_files;
	data_descriptor_t *files;
} data_config_t;

static data_config_t *data_load() {
	char path[MAX_PATH];
	get_user_config_folder(path, MAX_PATH, "SIMPLE");
	strcat(path, "data.conf");
	data_config_t *c = malloc(sizeof(data_config_t));
	c->known_files = 0;
	c->files = NULL;
	xml_document_t doc = h_document_read(path);
	if (doc) {
		xml_node_t r = document_get_root(doc);
		node_for_each(r, "Data", data, {
			data_descriptor_t file;
			const char *name = node_get_attribute(data, "name");
			const char *version = node_get_attribute(data, "version");
			const char *path = node_get_value(data);
			if (name != NULL && path != NULL) {
				c->known_files++;
				file.name = strdup(name);
				file.path = strdup(path);
				file.version = version ? strdup(version) : NULL;
				sb_push(c->files, file);
			}
			else {
				simple_report_error("Invalid data file specified!");
			}
		});
		document_destroy(doc);
	}
	return c;
}

static void data_free(data_config_t *config) {
	for (size_t i = 0; i < config->known_files; i++) {
		free(config->files[i].name);
		free(config->files[i].path);
		if (config->files[i].version) {
			free(config->files[i].version);
		}
	}
	sb_free(config->files);
	free(config);
}

static void data_save(data_config_t *config) {
	xml_document_t doc = h_document_create("SIMPLEData");
	xml_node_t root = document_get_root(doc);
	for (size_t i = 0; i < config->known_files; i++) {
		xml_node_t n = node_create(doc, "Data", config->files[i].path);
		node_set_attribute(n, "name", config->files[i].name);
		if (config->files[i].version) {
			node_set_attribute(n, "version", config->files[i].version);
		}
		node_append_child(root, n);
	}
	char path[MAX_PATH];
	get_user_config_folder(path, MAX_PATH, "SIMPLE");
	strcat(path, "data.conf");
	document_write(doc, path);
	document_destroy(doc);
}

void data_register(const char *const name, const char *const full_path, const char *const version) {
	data_config_t *c = data_load();
	if (name && full_path) {
		data_descriptor_t file;
		file.name = strdup(name);
		file.path = strdup(full_path);
		file.version = version ? strdup(version) : NULL;
		c->known_files++;
		sb_push(c->files, file);
	}
	data_save(c);
	data_free(c);
}

SIMPLEFile simple_data_file(const char *const name, const char *const version) {
	SIMPLEFile f;
	f.valid = false;
	data_config_t *c = data_load();
	for (size_t i = 0; i < c->known_files; i++) {
		if (!strcmp(c->files[i].name, name)) {
			if (version == NULL || (c->files[i].version
					&& strcmp(version, c->files[i].version) == 0)) {
				FILE *file = fopen(c->files[i].path, "rb");
				if (file) {
					fseek(file, 0, SEEK_END);
					f.length = ftell(file);
					rewind(file);
					f.buf = malloc(f.length + 1);
					if (fread(f.buf, 1, f.length, file) != f.length) {
						simple_report_error("Error reading file!");
						free(f.buf);
					}
					else {
						f.valid = true;
					}
					fclose(file);
				}
				break;
			}
		}
	}
	data_free(c);
	return f;
}

void simple_data_free(SIMPLEFile f) {
	if (f.valid) {
		free(f.buf);
		f.valid = false;
	}
}