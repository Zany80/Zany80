#pragma once

#include <stdbool.h>
#include <stdint.h>

/// general RapidXML binding

typedef struct _xml_node_t *xml_node_t;
typedef struct _xml_document_t *xml_document_t;

xml_document_t h_document_create(const char *root_name);
// cleans the document and all associated nodes
void document_destroy(xml_document_t doc);
xml_document_t h_document_read(const char *path);
bool document_write(xml_document_t doc, const char *path);
xml_node_t document_get_root(xml_document_t doc);

xml_node_t node_create(xml_document_t doc, const char *name, const char *value);
/// convenience function to allow using an integer as the value
xml_node_t node_create_i(xml_document_t doc, const char *name, int64_t value);
xml_node_t node_get_child(xml_node_t node, const char *name);
void node_for_each(xml_node_t node, const char *type, void (*handler)(xml_node_t));
xml_node_t node_get_next_sibling(xml_node_t node, const char *name);
xml_node_t node_get_prev_sibling(xml_node_t node, const char *name);
xml_node_t node_get_parent(xml_node_t node);
const char *node_get_attribute(xml_node_t node, const char *name);
int64_t node_get_attribute_i(xml_node_t node, const char *name);
const char *node_get_value(xml_node_t node);
const char *node_get_name(xml_node_t node);
/// convenience function to convert the value to an integer
int64_t node_get_value_i(xml_node_t);
void node_append_child(xml_node_t target, xml_node_t child);
void node_set_attribute(xml_node_t node, const char *name, const char *value);
void node_set_attribute_i(xml_node_t node, const char *name, int64_t valuei);
xml_document_t node_get_document(xml_node_t node);

#define node_for_each(node, type, name, handling_code) { \
    xml_node_t name = node_get_child(node, type); \
    while (name) { \
        handling_code; \
        name = node_get_next_sibling(name, type); \
    } \
}
