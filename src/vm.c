#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

struct var {
    const char* name;
    void* value;
    uint32_t scope;
};

struct var variable_table[1024];
uint32_t n_table = 0;

static int get_variable(const char* name, uint32_t* index) {
    for (uint32_t i = 0; i < n_table; ++i) {
        if (strcmp(name, variable_table[i].name) == 0) {
            *index = i;
            return 0;
        }
    }

    return 1;
}

static void add_variable(const char* var_name, uint32_t scope, uint32_t* current_stack_size) {
    while (*current_stack_size > 0 && variable_table[*current_stack_size - 1].scope > scope) {
        --*current_stack_size;
    }

    variable_table[n_table++] = (struct var) {
        .name = var_name,
        .scope = scope
    };

    ++*current_stack_size;
}

enum instructions {
    PUSH,
    POP,
    ADD,
    DUP,
    PRINT,
    CHANGE,
    JMP
};

#define add_instruction(code) \
{ \
    bytes[(*n_bytes)++] = code; \
}

int evaluate(struct node* ast, uint8_t* bytes, size_t* n_bytes, uint32_t* current_stack_size) {
    if (ast == NULL) {
        return 0;
    }

    uint32_t stack_s = 0;
    if (current_stack_size == NULL) {
        current_stack_size = &stack_s;
    }

    switch (ast->type) {
        case NODE_NUMBER:
            add_instruction(PUSH);

            memcpy(&bytes[*n_bytes], ast->token->value, sizeof(uint32_t));
            (*n_bytes) += sizeof(uint32_t);
            return 0;

        case NODE_VAR:
            {
                uint32_t position;
                int res = get_variable(ast->token->value, &position);
                if (res != 0) {
                    return 1;
                }

                add_instruction(DUP);

                memcpy(&bytes[*n_bytes], &position, sizeof(position));
                (*n_bytes) += sizeof(position);

                return 0;
            }

        case NODE_BINARY_OP:
            switch (ast->token->code) {
                case TOK_ADD:
                    evaluate(ast->left, bytes, n_bytes, current_stack_size);
                    evaluate(ast->right, bytes, n_bytes, current_stack_size);

                    add_instruction(ADD);
                    return 0;

                /* case TOK_MIN:
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
                    return evaluate(ast->left) || evaluate(ast->right); */

                default:
                    return 0;
            }
        case NODE_IF:
            {
                evaluate(ast->left, bytes, n_bytes, current_stack_size);
                evaluate(ast->right->left, bytes, n_bytes, current_stack_size);
                evaluate(ast->right->right, bytes, n_bytes, current_stack_size);

                return 0;
            }
        /*
        case NODE_WHILE:
            {
                int result = 0;
                while (evaluate(ast->left)) {
                    result = evaluate(ast->right->left);
                }

                return result;
            }
        */
        case NODE_ASSIGN:
            {
                const char* var_name = ast->left->token->value;

                uint32_t position;
                int res = get_variable(var_name, &position);
                if (res != 0) {
                    evaluate(ast->right, bytes, n_bytes, current_stack_size);
                    add_variable(var_name, ast->scope, current_stack_size);

                    return 0;
                }

                evaluate(ast->right, bytes, n_bytes, current_stack_size);

                add_instruction(CHANGE);

                memcpy(&bytes[*n_bytes], &position, sizeof(position));
                (*n_bytes) += sizeof(position);

                return 0;

            }
        case NODE_STATEMENT:
            {
                evaluate(ast->left, bytes, n_bytes, current_stack_size);
                evaluate(ast->right, bytes, n_bytes, current_stack_size);

                return 0;
            }

        case NODE_FUNCTION_CALL:
            {
                const char* function_name = ast->token->value;

                if (strcmp(function_name, "print") == 0) {
                    evaluate(ast->left, bytes, n_bytes, current_stack_size);

                    add_instruction(PRINT);

                    return 0;
                }

                /* struct var* v = var_from_mem(function_name);
                struct node* function_node = v->ref;

                return evaluate(function_node->right, NULL, NULL); */

                return 0;
            }

        /*
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
            } */
        default:
            return 0;
    }

    return 0;
}


int execute(struct vm* vm) {
    while (!vm->halt && vm->program_counter < vm->n_bytes) {
        switch (vm->bytes[vm->program_counter]) {
            case PUSH:
                vm->stack[vm->stack_size++] = *((uint32_t*)(vm->bytes + vm->program_counter + 1));
                vm->program_counter += sizeof(uint32_t);
                break;

            case POP:
                --vm->stack_size;
                break;

            case ADD:
                {
                    uint32_t number1 = vm->stack[--vm->stack_size];
                    uint32_t number2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = number1 + number2;
                }
                break;
            case PRINT:
                {
                    uint32_t number = vm->stack[--vm->stack_size];
                    printf("%d\n", number);
                }
                break;

            case DUP:
                {
                    uint32_t index = *((uint32_t*)(vm->bytes + vm->program_counter + 1));
                    vm->stack[vm->stack_size++] = vm->stack[index];
                    vm->program_counter += sizeof(uint32_t);
                }
                break;
            case CHANGE:
                {
                    uint32_t index = *((uint32_t*)(vm->bytes + vm->program_counter + 1));
                    vm->stack[index] = vm->stack[--vm->stack_size];
                    vm->program_counter += sizeof(uint32_t);
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

