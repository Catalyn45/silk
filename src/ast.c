#include "ast.h"
#include <stdlib.h>

struct node* node_new(int type, const struct token* token, struct node* left, struct node* right, uint32_t scope) {
    struct node* n = malloc(sizeof(*n));
    if (!n) {
        return NULL;
    }

    *n = (struct node) {
        .type = type,
        .token = token,
        .left = left,
        .right = right,
        .scope = scope
    };

    return n;
}

void node_free(struct node* n) {
    if (n->left)
        node_free(n->left);

    if (n->right)
        node_free(n->right);

    free(n);
}
