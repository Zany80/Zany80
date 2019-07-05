#include "privatize.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "functions.h"
#include "expression.h"
#include "objects.h"
#include "md5.h"
#include "runtime.h"
#include "stringop.h"

void rename_symbol(area_t *a, const char *original, const char *new) {
	int i;
	for (i = 0; i < a->late_immediates->length; ++i) {
		late_immediate_t *imm = a->late_immediates->items[i];
		int j;
		for (j = 0; j < imm->expression->tokens->length; ++j) {
			expression_token_t *tok = imm->expression->tokens->items[j];
			if (tok->type == SYMBOL) {
				if (strcasecmp(tok->symbol, original) == 0) {
					free(tok->symbol);
					tok->symbol = malloc(strlen(new) + 1);
					strcpy(tok->symbol, new);
				}
			}
		}
	}
	metadata_t *meta = get_area_metadata(a, "scas.functions");
	if (meta != NULL) {
		list_t *functions = decode_function_metadata(a, meta->value, meta->value_length);
		for (i = 0; i < functions->length; ++i) {
			function_metadata_t *func = functions->items[i];
			if (strcasecmp(func->name, original) == 0) {
				free(func->name);
				func->name = malloc(strlen(new) + 1);
				strcpy(func->name, new);
			}
			if (strcasecmp(func->start_symbol, original) == 0) {
				free(func->start_symbol);
				func->start_symbol = malloc(strlen(new) + 1);
				strcpy(func->start_symbol, new);
			}
			if (strcasecmp(func->end_symbol, original) == 0) {
				free(func->end_symbol);
				func->end_symbol = malloc(strlen(new) + 1);
				strcpy(func->end_symbol, new);
			}
		}
		meta->value = encode_function_metadata(functions, &meta->value_length);
		list_free(functions);
		set_area_metadata(a, "scas.functions", meta->value, meta->value_length);
	}
}

int contains_string(list_t *l, const char *s) {
	int i;
	for (i = 0; i < l->length; ++i) {
		if (strcasecmp(l->items[i], s) == 0) {
			return 1;
		}
	}
	return 0;
}

void privatize_area(object_t *o, area_t *a, list_t *exports) {
	MD5_CTX ctx;
	unsigned char raw_checksum[16];
	char checksum[33];

	MD5_Init(&ctx);
	MD5_Update(&ctx, a->data, a->data_length);
	MD5_Final(raw_checksum, &ctx);

	int i;
	for (i = 0; i < 16; ++i) {
		sprintf(checksum + (i * 2), "%02x", raw_checksum[i]);
	}

	scas_log(L_DEBUG, "Privatizing area %s with checksum %s", a->name, checksum);

	for (i = 0; i < a->symbols->length; ++i) {
		symbol_t *s = a->symbols->items[i];
		if (contains_string(exports, s->name)) {
			scas_log(L_DEBUG, "Found exported symbol '%s'", s->name);
			s->exported = 1;
		} else {
			char *new_name = malloc(
				strlen(s->name) +
				1 +
				strlen(checksum) +
				1);
			strcpy(new_name, s->name);
			strcat(new_name, "@");
			strcat(new_name, checksum);
			scas_log(L_DEBUG, "Renaming private symbol '%s' to '%s'", s->name, new_name);
			int j;
			for (j = 0; j < o->areas->length; ++j) {
				area_t *_a = o->areas->items[j];
				rename_symbol(_a, s->name, new_name);
			}
			free(s->name);
			s->name = new_name;
		}
	}
}

/* 
 * Should be done as the last step of assembly.
 * Converts unexported symbols into private symbols.
 */
void privatize_symbols(object_t *o) {
	if (!scas_runtime.options.explicit_export) {
		scas_log(L_DEBUG, "Skipping privitation due to explicit export being turned off.");
		return;
	}

	int i;
	for (i = 0; i < o->areas->length; ++i) {
		area_t *a = o->areas->items[i];
		privatize_area(o, a, o->exports);
	}
}
