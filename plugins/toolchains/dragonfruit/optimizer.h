#pragma once

#include "lexer.h"

typedef struct optimizer_t optimizer_t;

// This effectively transfers ownership of the passed lexer to the new lexer.
// Under no circumstances should the lexer passed in be accessed in any manner
// other than peek or insert.
optimizer_t *lexer_optimizer(lexer_t *lexer);
void optimizer_peek(optimizer_t *optimizer, char **token, lexer_token_t *type);
void optimizer_extract(optimizer_t *optimizer, char **token, lexer_token_t *type);
void optimizer_destroy(optimizer_t *optimizer);
