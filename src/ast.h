#ifndef AST_H
#define AST_H

struct node {
    int type;
    const struct token_entry* token;
    struct node* left;
    struct node* right;
};

struct node* node_new(int type, const struct token_entry* token, struct node* left, struct node* right);
void node_free(struct node* n);

#endif // AST_H
