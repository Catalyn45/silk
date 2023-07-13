#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

static int get_variable(const char* name, struct evaluator* e, uint32_t* index) {
    for (uint32_t i = 0; i < e->n_locals; ++i) {
        if (strcmp(name, e->locals[i].name) == 0) {
            *index = e->locals[i].stack_index;
            return 0;
        }
    }

    return 1;
}

static void add_variable(const char* var_name, uint32_t scope, uint32_t index, struct evaluator* e) {
    while (e->n_locals > 0 && e->locals[e->n_locals - 1].scope > scope) {
        --(e->n_locals);
    }

    e->locals[e->n_locals++] = (struct var) {
        .name = var_name,
        .scope = scope,
        .stack_index = index
    };
}

static void clean_variables(uint32_t scope, struct evaluator* e) {
    while (e->n_locals > 0 && e->locals[e->n_locals - 1].scope > scope) {
        --(e->n_locals);
    }
}

static struct function* get_function(const char* function_name, struct evaluator* e) {
    for (uint i = 0; i < e->n_functions; ++i) {
        if (strcmp(e->functions[i].name, function_name) == 0) {
            return &e->functions[i];
        }
    }

    return NULL;
}

static void add_function(const char* function_name, const char** parameters, uint32_t n_parameters, uint32_t index, struct evaluator* e) {
    e->functions[e->n_functions++] = (struct function) {
        .name = function_name,
        .parameters = parameters,
        .n_parameters = n_parameters,
        .index = index
    };
}

static uint32_t scope_push_count(struct evaluator* e, uint32_t scope) {
    uint32_t count = 0;
    uint32_t n = e->n_locals;

    while (n > 0 && e->locals[n - 1].scope > scope) {
        if (e->locals[n - 1].stack_index >= 0)
            count += 1;

        --n;
    }

    return count;
}

#define add_instruction(code) \
{ \
    bytes[(*n_bytes)++] = code; \
}

#define add_number(num) \
{ \
    int32_t int_value = (int32_t)num; \
    memcpy(&bytes[*n_bytes], &int_value, sizeof(int_value)); \
    (*n_bytes) += sizeof(num); \
}

#define increment_index() \
    ++(*current_stack_index)

int evaluate(struct node* ast, uint8_t* bytes, uint32_t* n_bytes, struct binary_data* data, uint32_t* current_stack_index, struct evaluator* e) {
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_NUMBER:
            {
                add_instruction(PUSH);

                memcpy(&bytes[*n_bytes], ast->token->value, sizeof(uint32_t));
                (*n_bytes) += sizeof(uint32_t);

                return 0;
            }
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
            evaluate(ast->right, bytes, n_bytes, data, current_stack_index, e);
            evaluate(ast->left, bytes, n_bytes, data, current_stack_index, e);

            switch (ast->token->code) {
                case TOK_ADD:
                    add_instruction(ADD);
                    break;

                case TOK_MIN:
                    add_instruction(MIN);
                    break;

                case TOK_MUL:
                    add_instruction(MUL);
                    break;

                case TOK_DIV:
                    add_instruction(DIV);
                    break;

                case TOK_LES:
                    add_instruction(LES);
                    break;

                case TOK_LEQ:
                    add_instruction(LEQ);
                    break;

                case TOK_GRE:
                    add_instruction(GRE);
                    break;

                case TOK_GRQ:
                    add_instruction(GRQ);
                    break;

                case TOK_DEQ:
                    add_instruction(DEQ);
                    break;

                case TOK_NEQ:
                    add_instruction(NEQ);
                    break;

                case TOK_AND:
                    add_instruction(AND);
                    break;

                case TOK_OR:
                    add_instruction(OR);
                    break;

                default:
                    return 1;
            }

            return 0;

        case NODE_IF:
            {
                // statements
                evaluate(ast->left, bytes, n_bytes, data, current_stack_index, e);
                add_instruction(JMP_NOT);

                uint32_t placeholder_true_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_true_index);

                // true
                evaluate(ast->right->left, bytes, n_bytes, data, current_stack_index, e);

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
                evaluate(ast->right->right, bytes, n_bytes, data, current_stack_index, e);

                for (uint32_t i = 0; i < scope_push_count(e, ast->scope); ++i)
                    add_instruction(POP);

                clean_variables(ast->scope, e);

                if (ast->right->right) {
                    memcpy(&bytes[placeholder_false_index], n_bytes, sizeof(*n_bytes));
                }

                return 0;
            }

        case NODE_WHILE:
            {
                uint32_t start_index = *n_bytes;

                // statements
                evaluate(ast->left, bytes, n_bytes, data, current_stack_index, e);
                add_instruction(JMP_NOT);

                uint32_t placeholder_false_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_false_index);

                // true
                evaluate(ast->right->left, bytes, n_bytes, data, current_stack_index, e);

                for (uint32_t i = 0; i < scope_push_count(e, ast->scope); ++i)
                    add_instruction(POP);

                clean_variables(ast->scope, e);

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
                    add_variable(var_name, ast->scope, *current_stack_index, e);
                    increment_index();

                    evaluate(ast->right, bytes, n_bytes, data, current_stack_index, e);

                    return 0;
                }

                evaluate(ast->right, bytes, n_bytes, data, current_stack_index, e);

                add_instruction(CHANGE);
                add_number(position);

                return 0;

            }
        case NODE_STATEMENT:
            {
                evaluate(ast->left, bytes, n_bytes, data, current_stack_index, e);
                evaluate(ast->right, bytes, n_bytes, data, current_stack_index, e);

                return 0;
            }

        case NODE_NOT:
            {
                evaluate(ast->left, bytes, n_bytes, data, current_stack_index, e);
                add_instruction(NOT);

                return 0;
            }

        case NODE_FUNCTION:
            {
                const char** parameters = malloc(1024 * sizeof(*parameters));
                if (!parameters) {
                    ERROR("error allocating memory");
                    return 1;
                }

                uint32_t n_parameters = 0;

                struct node* parameter = ast->left;
                while (parameter) {
                    add_variable(parameter->token->value, ast->scope + 1, -1 - n_parameters, e);

                    parameters[n_parameters++] = parameter->token->value;
                    parameter = parameter->left;
                }

                add_instruction(JMP);

                uint32_t placeholder_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_index);

                const char* fun_name = ast->token->value;
                add_function(fun_name, parameters, n_parameters, *n_bytes, e);

                // the first 2 are old base and return address
                uint32_t new_stack_index = 2;
                evaluate(ast->right, bytes, n_bytes, data, &new_stack_index,  e);

                for (uint32_t i = 0; i < scope_push_count(e, ast->scope); ++i) {
                    add_instruction(POP);
                }

                add_instruction(RET);

                memcpy(&bytes[placeholder_index], n_bytes, sizeof(*n_bytes));

                return 0;
            }

        case NODE_FUNCTION_CALL:
            {
                const char* function_name = ast->token->value;
                if (strcmp(function_name, "print") == 0) {
                    evaluate(ast->left->left, bytes, n_bytes, data, current_stack_index, e);

                    add_instruction(PRINT);

                    return 0;
                }

                struct function* f = get_function(function_name, e);
                if (!f) {
                    ERROR("function not definited");
                    return 1;
                }

                struct node* arguments = ast->left;
                struct node* argument = arguments;

                uint32_t n_arguments = 0;
                struct node* arg_list[256];

                while (argument) {
                    arg_list[n_arguments++] = argument->left;
                    argument = argument->right;
                }

                for (int32_t i = n_arguments - 1; i >= 0; --i) {
                    evaluate(arg_list[i], bytes, n_bytes, data, current_stack_index, e);
                }

                add_instruction(CALL);
                add_number(f->index);

                for (uint32_t i = 0; i < n_arguments; ++i) {
                    add_instruction(POP);
                }

                return 0;
            }

        default:
            return 0;
    }

    return 0;
}

#define read_value(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1)));

#define push(value) \
    vm->stack[vm->stack_size++] = value;

#define pop() \
    vm->stack[--vm->stack_size];

int execute(struct vm* vm) {
    while (!vm->halt && vm->program_counter < vm->n_bytes) {
        switch (vm->bytes[vm->program_counter]) {
            case PUSH:
                push(read_value(int32_t))
                vm->program_counter += sizeof(uint32_t);
                break;

            case POP:
                --vm->stack_size;
                break;

            case ADD:
                {
                    uint32_t number1 = pop();
                    uint32_t number2 = pop();
                    push(number1 + number2);
                }
                break;

            case MIN:
                {
                    uint32_t number1 = pop();
                    uint32_t number2 = pop();
                    push(number1 - number2);
                }
                break;

            case MUL:
                {
                    uint32_t number1 = vm->stack[--vm->stack_size];
                    uint32_t number2 = vm->stack[--vm->stack_size];
                    push(number1 * number2);
                }
                break;

            case DIV:
                {
                    uint32_t number1 = vm->stack[--vm->stack_size];
                    uint32_t number2 = vm->stack[--vm->stack_size];
                    push(number1 / number2);
                }
                break;

            case NOT:
                {
                    uint32_t exp = vm->stack[--vm->stack_size];
                    if (exp) {
                        push(0);
                    } else {
                        push(1);
                    }
                }
                break;

            case DEQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 == exp2) ? 1 : 0);
                }
                break;

            case NEQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 != exp2) ? 1 : 0);
                }
                break;

            case GRE:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 > exp2) ? 1 : 0);
                }
                break;

            case GRQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 >= exp2) ? 1 : 0);
                }
                break;

            case LES:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 < exp2) ? 1 : 0);
                }
                break;

            case LEQ:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 <= exp2) ? 1 : 0);
                }
                break;

            case AND:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 && exp2) ? 1 : 0);
                }
                break;

            case OR:
                {
                    uint32_t exp1 = vm->stack[--vm->stack_size];
                    uint32_t exp2 = vm->stack[--vm->stack_size];
                    push((exp1 || exp2) ? 1 : 0);
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
                    int32_t index = read_value(int32_t)
                    push(vm->stack[vm->stack_base + index]);
                    vm->program_counter += sizeof(index);
                }
                break;
            case CHANGE:
                {
                    int32_t index = read_value(int32_t)
                    vm->stack[vm->stack_base + index] = vm->stack[--vm->stack_size];
                    vm->program_counter += sizeof(index);
                }
                break;
            case JMP_NOT:
                {
                    uint32_t condition = vm->stack[--vm->stack_size];
                    int32_t index = read_value(int32_t);

                    if (!condition) {
                        vm->program_counter = index - 1;
                    } else {
                        vm->program_counter += sizeof(index);
                    }
                }
                break;

            case JMP:
                {
                    int32_t index = read_value(int32_t);
                    vm->program_counter = index - 1;
                }
                break;

            case CALL:
                {
                    uint32_t old_base = vm->stack_base;
                    vm->stack_base = vm->stack_size;

                    push(vm->program_counter + 1 + sizeof(int32_t))

                    int32_t index = read_value(int32_t);
                    vm->program_counter = index - 1;

                    push(old_base);
                }
                break;

            case RET:
                {
                    vm->stack_base = pop();
                    uint32_t index = pop();
                    vm->program_counter = index - 1;
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

