#ifndef AST_H
#define AST_H

#include <stdint.h>

struct node {
    int type;
    uint32_t scope;
    const struct token_entry* token;
    struct node* left;
    struct node* right;
    uint32_t flags;
};

struct node* node_new(int type, const struct token_entry* token, struct node* left, struct node* right, uint32_t scope);
void node_free(struct node* n);

#endif // AST_H
