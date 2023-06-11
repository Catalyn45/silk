#ifndef PARSER_H_
#define PARSER_H_

#include "lexer.h"
#include "ast.h"

enum node_type {
    NODE_NUMBER,
    NODE_BINARY_OP,
    NODE_IF,
    NODE_WHILE,
    NODE_ASSIGN,
    NODE_STATEMENT,
    NODE_VAR,
    NODE_FUNCTION_CALL
};

int parse (const struct token_entry* tokens, uint32_t n_tokens, struct node** root);

#endif
