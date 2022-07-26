#ifndef PARSER_H_
#define PARSER_H_

#include <lexer.h>

enum node_type {
    NODE_NUMBER,
    NODE_BINARY_OP
};

struct node {
    int type;
    void* data;
};

struct number_data {
    struct token_entry* token;
};

struct binary_op_data {
    struct node* left_op;
    struct token_entry* token;
    struct node* right_op;
};

int parse ( const struct token_entry* tokens, uint32_t n_tokens, struct node* root );

#endif
