#include "parser.h"
#include <stdlib.h>

#define get_token() \
    &tokens[*current_index]

#define advance() \
    ++(*current_index)

static int factor(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    (void)n_tokens;
    const struct token_entry* current_token = get_token();
    if (current_token->token_code != TOK_INT) {
        // TODO: handle
        return 1;
    }

    struct node* node_num = malloc(sizeof(*node_num));
    if (node_num == NULL) {
        // TODO: handle
        return 1;
    }

    node_num->type = NODE_NUMBER;
    node_num->token = current_token;
    node_num->left = NULL;
    node_num->right = NULL;

    advance();

    *root = node_num;

    return 0;
}

static int term(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* left = NULL;
    int result = factor(current_index, tokens, n_tokens, &left);
    if ( result != 0 ) {
        // TODO: handle
        return 1;
    }

    const struct token_entry* current_token = get_token();
    while (current_token->token_code == TOK_MUL || current_token->token_code == TOK_DIV) {
        advance();
        struct node* right = NULL;
        result = factor(current_index, tokens, n_tokens, &right);
        if ( result != 0 ) {
            // TODO: handle
            return 1;
        }

        struct node* bin_op = malloc(sizeof(*bin_op));
        if (bin_op == NULL) {
            // TODO: handle
            return 1;
        }

        bin_op->type = NODE_BINARY_OP;
        bin_op->token = current_token;
        bin_op->left = left;
        bin_op->right = right;

        left = bin_op;

        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int expression(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* left = NULL;
    int result = term(current_index, tokens, n_tokens, &left);
    if ( result != 0 ) {
        // TODO: handle
        return 1;
    }

    const struct token_entry* current_token = get_token();
    while (current_token->token_code == TOK_ADD || current_token->token_code == TOK_MIN) {
        advance();
        struct node* right = NULL;
        result = term(current_index, tokens, n_tokens, &right);
        if ( result != 0 ) {
            // TODO: handle
            return 1;
        }

        struct node* bin_op = malloc(sizeof(*bin_op));
        if (bin_op == NULL) {
            // TODO: handle
            return 1;
        }

        bin_op->type = NODE_BINARY_OP;
        bin_op->token = current_token;
        bin_op->left = left;
        bin_op->right = right;

        left = bin_op;

        current_token = get_token();
    }

    *root = left;

    return 0;
}

int parse ( const struct token_entry* tokens, uint32_t n_tokens, struct node** root ) {
    for (uint32_t i = 0; i < n_tokens; i++) {
        if (expression(&i, tokens, n_tokens, root) != 0) {
            // TODO: error handling
            return 1;
        }
    }

    return 0;
}
