#include "optimizer.h"
#include "internals.h"
#include <stdlib.h>

optimizer_t *lexer_optimizer(lexer_t *lexer) {
	optimizer_t *optimizer = malloc(sizeof(optimizer_t));
	optimizer->lexer = lexer;
	return optimizer;
}

void optimizer_peek(optimizer_t *optimizer, char **token, lexer_token_t *type) {
	lexer_peek(optimizer->lexer, token, type);
}

void optimizer_extract(optimizer_t *optimizer, char **token, lexer_token_t *type) {
	lexer_extract(optimizer->lexer, token, type);	
}

void optimizer_destroy(optimizer_t *optimizer) {
	free(optimizer);
}
