#define RAPIDXML_NO_EXCEPTIONS
#include "rapidxml.hpp"
using namespace rapidxml;

typedef struct _xml_node_t *xml_node_t;
typedef struct _xml_document_t *xml_document_t;

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
