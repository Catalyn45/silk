#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

struct var {
    const char* name;
    int value;
    void* ref;
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
            return *((int*)ast->token->value);
        case NODE_VAR:
            return var_from_mem(ast->token->value)->value;

        case NODE_BINARY_OP:
            switch (ast->token->code) {
                case TOK_ADD:
                    return evaluate(ast->left) + evaluate(ast->right);
                case TOK_MIN:
                    return evaluate(ast->left) - evaluate(ast->right);
                case TOK_MUL:
                    return evaluate(ast->left) * evaluate(ast->right);
                case TOK_DIV:
                    return evaluate(ast->left) / evaluate(ast->right);
                case TOK_LES:
                    return evaluate(ast->left) < evaluate(ast->right);
                case TOK_LEQ:
                    return evaluate(ast->left) <= evaluate(ast->right);
                case TOK_GRE:
                    return evaluate(ast->left) > evaluate(ast->right);
                case TOK_GRQ:
                    return evaluate(ast->left) >= evaluate(ast->right);
                case TOK_DEQ:
                    return evaluate(ast->left) == evaluate(ast->right);
                case TOK_NEQ:
                    return evaluate(ast->left) != evaluate(ast->right);
                case TOK_AND:
                    return evaluate(ast->left) && evaluate(ast->right);
                case TOK_OR:
                    return evaluate(ast->left) || evaluate(ast->right);

                default:
                    return 0;
            }
        case NODE_IF:
            {
                int result = evaluate(ast->left);
                if (result) {
                    return evaluate(ast->right->left);
                }

                if (ast->right->right) {
                    return evaluate(ast->right->right);
                }
            }
        case NODE_WHILE:
            {
                int result = 0;
                while (evaluate(ast->left)) {
                    result = evaluate(ast->right->left);
                }

                return result;
            }
        case NODE_ASSIGN:
            {
                const char* var_name = ast->left->token->value;

                struct var* v = var_from_mem(var_name);
                if (v == NULL) {
                    memory[used_mem++] = (struct var) {
                        .name = var_name,
                        .value = evaluate(ast->right)
                    };
                    return 0;
                }

                v->value = evaluate(ast->right);
                return v->value;

            }
        case NODE_STATEMENT:
            {
                return evaluate(ast->left) + evaluate(ast->right);
            }
        case NODE_FUNCTION_CALL:
            {
                const char* function_name = ast->token->value;

                if (strcmp(function_name, "print") == 0) {
                    int parameter = evaluate(ast->left);
                    printf("%d\n", parameter);

                    return 0;
                }

                struct var* v = var_from_mem(function_name);
                struct node* function_node = v->ref;

                return evaluate(function_node->right);

            }
        case NODE_NOT:
            {
                return !evaluate(ast->left);
            }
        case NODE_FUNCTION:
            {
                const char* fun_name = ast->token->value;
                memory[used_mem++] = (struct var) {
                    .name = fun_name,
                    .ref = (void*)ast
                };

                return 0;
            }
        default:
            return 0;
    }

    return 0;
}
