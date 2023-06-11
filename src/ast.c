#include "ast.h"
#include <stdlib.h>

struct node* node_new(int type, const struct token_entry* token, struct node* value, struct node* left, struct node* right) {
    struct node* n = malloc(sizeof(*n));
    if (!n) {
        return NULL;
    }

    *n = (struct node) {
        .type = type,
        .token = token,
        .value = value,
        .left = left,
        .right = right
    };

    return n;
}

void node_free(struct node* n) {
    if (n->value)
        node_free(n->value);

    if (n->left)
        node_free(n->left);

    if (n->right)
        node_free(n->right);

    free(n);
}
