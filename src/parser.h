#ifndef PARSER_H_
#define PARSER_H_

#include "lexer.h"
#include "ast.h"

enum node_type {
    NODE_NUMBER = 0,
    NODE_BINARY_OP = 1,
    NODE_IF = 2,
    NODE_WHILE = 3,
    NODE_ASSIGN = 4,
    NODE_STATEMENT = 5,
    NODE_VAR = 6,
    NODE_FUNCTION_CALL = 7,
    NODE_NOT = 8,
    NODE_FUNCTION = 9,
    NODE_DECISION = 10
};

int parse(const char* text, const struct token_entry* tokens, uint32_t n_tokens, struct node** root );

#endif
