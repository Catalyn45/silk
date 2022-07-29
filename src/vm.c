#include "vm.h"
#include "stdlib.h"


int evaluate(struct node* ast) {
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_NUMBER:
            return *((int*)ast->token->token_value);
        case NODE_BINARY_OP:
            switch (ast->token->token_code) {
                case TOK_ADD:
                    return evaluate(ast->left) + evaluate(ast->right);
                case TOK_MIN:
                    return evaluate(ast->left) - evaluate(ast->right);
                case TOK_MUL:
                    return evaluate(ast->left) * evaluate(ast->right);
                case TOK_DIV:
                    return evaluate(ast->left) / evaluate(ast->right);
                default:
                    return 0;
            }
        default:
            return 0;
    }

    return 0;
}
