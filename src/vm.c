#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static int get_variable(const char* name, struct evaluator* e, uint32_t* index) {
    for (uint32_t i = 0; i < e->n_locals; ++i) {
        if (strcmp(name, e->locals[i].name) == 0) {
            *index = i;
            return 0;
        }
    }

    return 1;
}

static void add_variable(const char* var_name, uint32_t scope, struct evaluator* e) {
    while (e->n_locals > 0 && e->locals[e->n_locals - 1].scope > scope) {
        --(e->n_locals);
    }

    e->locals[e->n_locals++] = (struct var) {
        .name = var_name,
        .scope = scope
    };
}

static uint32_t scope_push_count(struct evaluator* e, uint32_t scope) {
    uint32_t count = 0;
    uint32_t n = e->n_locals;

    while (n > 0 && e->locals[n - 1].scope > scope) {
        count += 1;
        --n;
    }

    return count;
}

enum instructions {
    PUSH,
    POP,
    ADD,
    MIN,
    MUL,
    DIV,
    NOT,
    DEQ,
    NEQ,
    GRE,
    GRQ,
    LES,
    LEQ,
    AND,
    OR,
    DUP,
    PRINT,
    CHANGE,
    JMP_NOT,
    JMP,
};

#define add_instruction(code) \
{ \
    bytes[(*n_bytes)++] = code; \
}

#define add_number(num) \
{ \
    memcpy(&bytes[*n_bytes], &num, sizeof(num)); \
    (*n_bytes) += sizeof(num); \
}

int evaluate(struct node* ast, uint8_t* bytes, uint32_t* n_bytes, struct evaluator* e) {
    if (ast == NULL) {
        return 0;
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

                int res = get_variable(ast->token->value, e, &position);
                if (res != 0) {
                    return 1;
                }

                add_instruction(DUP);
                add_number(position);

                return 0;
            }

        case NODE_BINARY_OP:
            switch (ast->token->code) {
                case TOK_ADD:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(ADD);
                    return 0;

                case TOK_MIN:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(MIN);
                    return 0;

                case TOK_MUL:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(MUL);
                    return 0;

                case TOK_DIV:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(DIV);
                    return 0;

                case TOK_LES:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(LES);
                    return 0;

                case TOK_LEQ:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(LEQ);
                    return 0;

                case TOK_GRE:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(GRE);
                    return 0;

                case TOK_GRQ:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(GRQ);
                    return 0;

                case TOK_DEQ:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(DEQ);
                    return 0;

                case TOK_NEQ:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(NEQ);
                    return 0;

                case TOK_AND:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(AND);
                    return 0;

                case TOK_OR:
                    evaluate(ast->right, bytes, n_bytes, e);
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(OR);
                    return 0;

                default:
                    return 0;
            }

        case NODE_IF:
            {
                // statements
                evaluate(ast->left, bytes, n_bytes, e);
                add_instruction(JMP_NOT);

                uint32_t placeholder_true_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_true_index);

                // true
                evaluate(ast->right->left, bytes, n_bytes, e);

                for (uint32_t i = 0; i < scope_push_count(e, ast->scope); ++i)
                    add_instruction(POP);

                uint32_t placeholder_false_index;
                if (ast->right->right) {
                    add_instruction(JMP);
                    placeholder_false_index = *n_bytes;
                    (*n_bytes) += sizeof(placeholder_false_index);
                }

                // fill placeholder
                memcpy(&bytes[placeholder_true_index], n_bytes, sizeof(*n_bytes));

                // false
                evaluate(ast->right->right, bytes, n_bytes, e);

                for (uint32_t i = 0; i < scope_push_count(e, ast->scope); ++i)
                    add_instruction(POP);

                if (ast->right->right) {
                    memcpy(&bytes[placeholder_false_index], n_bytes, sizeof(*n_bytes));
                }

                return 0;
            }

        case NODE_WHILE:
            {
                uint32_t start_index = *n_bytes;

                // statements
                evaluate(ast->left, bytes, n_bytes, e);
                add_instruction(JMP_NOT);

                uint32_t placeholder_false_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_false_index);

                // true
                evaluate(ast->right->left, bytes, n_bytes, e);

                for (uint32_t i = 0; i < scope_push_count(e, ast->scope); ++i)
                    add_instruction(POP);

                add_instruction(JMP);
                add_number(start_index);

                // fill placeholder
                memcpy(&bytes[placeholder_false_index], n_bytes, sizeof(*n_bytes));

                return 0;
            }

        case NODE_ASSIGN:
            {
                const char* var_name = ast->left->token->value;

                uint32_t position;
                int res = get_variable(var_name, e, &position);
                if (res != 0) {
                    add_variable(var_name, ast->scope, e);
                    evaluate(ast->right, bytes, n_bytes, e);

                    return 0;
                }

                evaluate(ast->right, bytes, n_bytes, e);

                add_instruction(CHANGE);
                add_number(position);

                return 0;

            }
        case NODE_STATEMENT:
            {
                evaluate(ast->left, bytes, n_bytes, e);
                evaluate(ast->right, bytes, n_bytes, e);

                return 0;
            }

        case NODE_FUNCTION_CALL:
            {
                const char* function_name = ast->token->value;

                if (strcmp(function_name, "print") == 0) {
                    evaluate(ast->left, bytes, n_bytes, e);

                    add_instruction(PRINT);

                    return 0;
                }

                /*

                struct var* v = var_from_mem(function_name);
                struct node* function_node = v->ref;

                return evaluate(function_node->right, NULL, NULL);

                */
                return 0;
            }

        case NODE_NOT:
            {
                evaluate(ast->left, bytes, n_bytes, e);
                add_instruction(NOT);

                return 0;
            }
        /*
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

#define read_value(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1)));

int execute(struct vm* vm) {
    while (!vm->halt && vm->program_counter < vm->n_bytes) {
        switch (vm->bytes[vm->program_counter]) {
            case PUSH:
                vm->stack[vm->stack_size++] = read_value(uint32_t)
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

            case MIN:
                {
                    uint32_t number1 = vm->stack[--vm->stack_size];
                    uint32_t number2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = number1 - number2;
                }
                break;

            case MUL:
                {
                    uint32_t number1 = vm->stack[--vm->stack_size];
                    uint32_t number2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = number1 * number2;
                }
                break;

            case DIV:
                {
                    uint32_t number1 = vm->stack[--vm->stack_size];
                    uint32_t number2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = number1 / number2;
                }
                break;

            case NOT:
                {
                    uint32_t exp = vm->stack[--vm->stack_size];
                    if (exp) {
                        vm->stack[vm->stack_size++] = 0;
                    } else {
                        vm->stack[vm->stack_size++] = 1;
                    }
                }
                break;

            case DEQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 == exp2) ? 1 : 0;
                }
                break;

            case NEQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 != exp2) ? 1 : 0;
                }
                break;

            case GRE:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 > exp2) ? 1 : 0;
                }
                break;

            case GRQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 >= exp2) ? 1 : 0;
                }
                break;

            case LES:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 < exp2) ? 1 : 0;
                }
                break;

            case LEQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 <= exp2) ? 1 : 0;
                }
                break;

            case AND:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 && exp2) ? 1 : 0;
                }
                break;

            case OR:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    vm->stack[vm->stack_size++] = (exp1 || exp2) ? 1 : 0;
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
                    uint32_t index = read_value(uint32_t)
                    vm->stack[vm->stack_size++] = vm->stack[index];
                    vm->program_counter += sizeof(index);
                }
                break;
            case CHANGE:
                {
                    uint32_t index = read_value(uint32_t)
                    vm->stack[index] = vm->stack[--vm->stack_size];
                    vm->program_counter += sizeof(index);
                }
                break;
            case JMP_NOT:
                {
                    uint32_t condition = vm->stack[--vm->stack_size];
                    uint32_t index = read_value(uint32_t);

                    if (!condition) {
                        vm->program_counter = index - 1;
                    } else {
                        vm->program_counter += sizeof(index);
                    }
                }
                break;

            case JMP:
                {
                    uint32_t index = read_value(uint32_t);
                    vm->program_counter = index - 1;
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

