#include <stdlib.h>

#include "ast.h"

struct node* node_new(int type, const struct token* token, struct node* left, struct node* right) {
    struct node* n = malloc(sizeof(*n));
    if (!n) {
        return NULL;
    }

    *n = (struct node) {
        .type = type,
        .left = left,
        .right = right,
    };

    if (token) {
        n->token = *token;
    }

    return n;
}

void node_free(struct node* n) {
    if (n->left)
        node_free(n->left);

    if (n->right)
        node_free(n->right);

    free(n);
}
