#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

struct var {
    const char* name;
    int value;
};

struct var memory[1024] = {};
uint32_t used_mem = 0;

struct var* var_from_mem(const char* name) {
    for (uint32_t i = 0; i < used_mem; ++i) {
        if (strcmp(name, memory[i].name) == 0) {
            return &memory[i];
        }
    }

    return NULL;
}

int evaluate(struct node* ast) {
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_NUMBER:
            return *((int*)ast->token->token_value);
        case NODE_VAR:
            return var_from_mem(ast->token->token_value)->value;

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
                case TOK_LES:
                    return evaluate(ast->left)< evaluate(ast->right);
                case TOK_LEQ:
                    return evaluate(ast->left) <= evaluate(ast->right);
                case TOK_GRE:
                    return evaluate(ast->left) > evaluate(ast->right);
                case TOK_GRQ:
                    return evaluate(ast->left) >= evaluate(ast->right);
                case TOK_DEQ:
                    return evaluate(ast->left) == evaluate(ast->right);
                case TOK_AND:
                    return evaluate(ast->left) && evaluate(ast->right);
                case TOK_OR:
                    return evaluate(ast->left) || evaluate(ast->right);

                default:
                    return 0;
            }
        case NODE_IF:
            {
                int result = evaluate(ast->value);
                if (result) {
                    return evaluate(ast->left);
                }

                if (ast->right) {
                    return evaluate(ast->right);
                }
            }
        case NODE_WHILE:
            {
                int result = 0;
                while (evaluate(ast->value)) {
                    result = evaluate(ast->left);
                }

                return result;
            }
        case NODE_ASSIGN:
            {
                struct var* v = var_from_mem(ast->token->token_value);
                if (v == NULL) {
                    memory[used_mem++] = (struct var) {
                        .name = ast->token->token_value,
                        .value = evaluate(ast->value)
                    };
                    return 0;
                }

                v->value = evaluate(ast->value);
                return v->value;

            }
        case NODE_STATEMENT:
            {
                return evaluate(ast->value) + evaluate(ast->left);
            }
        case NODE_FUNCTION_CALL:
            {
                const char* function_name = ast->token->token_value;

                if (strcmp(function_name, "print") != 0) {
                    return 1;
                }

                int parameter = evaluate(ast->value);
                printf("%d\n", parameter);

                return 0;
            }
        case NODE_NOT:
            {
                return !evaluate(ast->value);
            }
        default:
            return 0;
    }

    return 0;
}
