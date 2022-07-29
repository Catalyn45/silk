#ifndef PARSER_H_
#define PARSER_H_

#include "lexer.h"

enum node_type {
    NODE_NUMBER,
    NODE_BINARY_OP
};

struct node {
    int type;
    const struct token_entry* token;
    struct node* left;
    struct node* right;
};

int parse ( const struct token_entry* tokens, uint32_t n_tokens, struct node** root );

#endif
