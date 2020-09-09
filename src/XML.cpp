#include "XML.h"

#define RAPIDXML_NO_EXCEPTIONS
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
using namespace rapidxml;

#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <inttypes.h>
jmp_buf env;

#include <fstream>

#include "stb/stb_ds.h"

struct _xml_document_t {
    xml_document<> *_doc;
    char *buf;
    xml_node_t *associated_nodes;
};

struct _xml_node_t {
    xml_node<> *_node;
    // doc is the parent document of this node, sourced is the owner of the doc
    // that is sourced for nodes with a src attribute
    xml_document_t doc, sourced;
};


char *h_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    assert(fseek(f, 0, SEEK_END) == 0);
    size_t size = ftell(f);
    rewind(f);
    char *buf = new char[size + 1];
    assert(fread(buf, 1, size, f) == size);
    fclose(f);
    buf[size] = 0;
    return buf;
}

xml_document_t h_document_read(const char *path) {
    xml_document_t xml = new struct _xml_document_t;
	xml->buf = h_read_file(path);
    xml->associated_nodes = NULL;
    if (xml->buf) {
        printf("Read XML file: %s\n", path);
    }
    else {
        printf("Failed to read XML file: %s\n", path);
        delete xml;
        return NULL;
    }
    xml->_doc = new xml_document<>;
    if (setjmp(env) == 0) {
        xml->_doc->parse<0>(xml->buf);
        return xml;
    }
    else {
        delete xml->_doc;
        delete[] xml->buf;
        delete xml;
        return NULL;
    }
}

xml_document_t h_document_create(const char *root_name) {
	xml_document_t xml = new struct _xml_document_t;
    xml->buf = NULL;
    xml->associated_nodes = NULL;
    xml->_doc = new xml_document<>;
    xml->_doc->append_node(xml->_doc->allocate_node(node_element, xml->_doc->allocate_string(root_name), NULL));
    return xml;
}

void document_destroy(xml_document_t doc) {
    if (doc->_doc != NULL) {
        doc->_doc->clear();
        delete doc->_doc;
    }
    delete[] doc->buf;
    for (size_t i = 0; i < stbds_arrlenu(doc->associated_nodes); i++) {
        if (doc->associated_nodes[i]->sourced != NULL) {
            document_destroy(doc->associated_nodes[i]->sourced);
        }
        delete doc->associated_nodes[i];
    }
    stbds_arrfree(doc->associated_nodes);
    delete doc;
}

xml_node_t document_get_root(xml_document_t doc) {
    xml_node<> *node = doc->_doc->first_node();
    if (node) {
        xml_node_t n = new struct _xml_node_t;
        n->_node = doc->_doc->first_node();
        n->doc = doc;
        n->sourced = NULL;
        stbds_arrpush(doc->associated_nodes, n);
        return n;
    }
    return NULL;
}

bool document_write(xml_document_t doc, const char *path) {
	std::ofstream o(path);
    if (o.is_open()) {
        o << *doc->_doc;
        o.flush();
        return true;
    }
    return false;
}

xml_node_t node_create(xml_document_t doc, const char *name, const char *value) {
	xml_node_t n = new struct _xml_node_t;
    n->doc = doc;
    n->sourced = NULL;
    n->_node = doc->_doc->allocate_node(node_element, doc->_doc->allocate_string(name), value ? doc->_doc->allocate_string(value) : NULL);
    stbds_arrpush(doc->associated_nodes, n);
    return n;
}

xml_node_t node_create_i(xml_document_t doc, const char *name, int64_t ivalue) {
    char value[32];
    sprintf(value, "%" PRIi64, ivalue);
    return node_create(doc, name, value);
}

int64_t node_get_value_i(xml_node_t node) {
    int64_t valuei = 0;
    sscanf(node->_node->value(), "%" SCNi64, &valuei);
    return valuei;
}

xml_node_t node_get_child(xml_node_t node, const char *name) {
    xml_node<> *child = node->_node->first_node(name);
    while (child != NULL) {
        if (child->type() == node_element) {
            xml_node_t n = new struct _xml_node_t;
            n->_node = child;
            n->doc = node->doc;
            n->sourced = NULL;
            stbds_arrpush(node->doc->associated_nodes, n);
            return n;
        }
        else {
            child = child->next_sibling(name);
        }
    }
    return NULL;
}

xml_node_t node_get_next_sibling(xml_node_t node, const char *name) {
    xml_node<> *sib = node->_node->next_sibling(name);
    if (sib) {
        xml_node_t n = new struct _xml_node_t;
        n->doc = node->doc;
        n->sourced = NULL;
        n->_node = sib;
        stbds_arrpush(n->doc->associated_nodes, n);
        return n;
    }
    return NULL;
}

xml_node_t node_get_parent(xml_node_t node) {
    xml_node<> *parent = node->_node->parent();
    if (parent) {
        xml_node_t n = new struct _xml_node_t;
        n->doc = node->doc;
        n->sourced = NULL;
        n->_node = parent;
        stbds_arrpush(n->doc->associated_nodes, n);
        return n;
    }
    return NULL;
}

const char *node_get_attribute(xml_node_t node, const char *name) {
    xml_attribute<> *attr = node->_node->first_attribute(name);
	return attr != NULL ? attr->value() : NULL;
}

int64_t node_get_attribute_i(xml_node_t node, const char *name) {
    xml_attribute<> *attr = node->_node->first_attribute(name);
	const char *str = attr != NULL ? attr->value() : NULL;
    int64_t value = 0;
    if (str) {
        value = atoi(str);
    }
    return value;
}

const char *node_get_value(xml_node_t node) {
	return node->_node->value();
}

const char *node_get_name(xml_node_t node) {
    return node->_node->name();
}

void node_append_child(xml_node_t target, xml_node_t child) {
	target->_node->append_node(child->_node);
}

void node_set_attribute(xml_node_t node, const char *name, const char *value) {
    if (name != NULL) {
        xml_attribute<> *a = node->_node->first_attribute(name);
        xml_document<> *d = node->doc->_doc;
        if (a) {
            if (value != NULL) {
                a->value(d->allocate_string(value));
            }
            else {
                node->_node->remove_attribute(a);
            }
        }
        else if (value != NULL) {
            node->_node->append_attribute(d->allocate_attribute(d->allocate_string(name), d->allocate_string(value)));
        }
    }
}

void node_set_attribute_i(xml_node_t node, const char *name, int64_t valuei) {
    char buf[32];
    sprintf(buf, "%" PRIi64, valuei);
    node_set_attribute(node, name, buf);
}

void rapidxml::parse_error_handler(const char *what, void *where) {
    printf("Error parsing XML: %s@%s\n", what, (char*)where);
    longjmp(env, 1);
}
