#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "lexer.h"

struct node {
    int type;
    struct token token;
    struct node* left;
    struct node* right;
    uint32_t flags;
};

struct node* node_new(int type, const struct token* token, struct node* left, struct node* right);
void node_free(struct node* n);

#endif // AST_H
