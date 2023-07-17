#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

static int get_variable(const char* name, struct evaluator* e, int32_t* index) {
    uint32_t i = e->n_locals;

    while (i > 0) {
        --i;

        if (strcmp(name, e->locals[i].name) == 0) {
            *index = e->locals[i].stack_index;
            return 0;
        }
    }

    return 1;
}

static void add_variable(const char* var_name, uint32_t scope, uint32_t index, struct evaluator* e) {
    e->locals[e->n_locals++] = (struct var) {
        .name = var_name,
        .scope = scope,
        .stack_index = index
    };
}

static uint32_t pop_variables(uint32_t scope, struct evaluator* e) {
    uint32_t count = 0;

    while (e->n_locals > 0 && e->locals[e->n_locals - 1].scope > scope) {
        if (e->locals[e->n_locals - 1].stack_index >= 0)
            count += 1;

        --(e->n_locals);
    }

    return count;
}

static struct function* get_function(const char* function_name, struct evaluator* e) {
    for (uint i = 0; i < e->n_functions; ++i) {
        if (strcmp(e->functions[i].name, function_name) == 0) {
            return &e->functions[i];
        }
    }

    return NULL;
}

static void add_function(const char* function_name, uint32_t n_parameters, uint32_t index, struct evaluator* e) {
    e->functions[e->n_functions++] = (struct function) {
        .name = function_name,
        .n_parameters = n_parameters,
        .index = index
    };
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
                int32_t position;
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

                uint32_t n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i)
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

                n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i)
                    add_instruction(POP);

                if (ast->right->right) {
                    memcpy(&bytes[placeholder_false_index], n_bytes, sizeof(*n_bytes));
                }

                return 0;
            }

        case NODE_WHILE:
            {
                uint32_t start_index = *n_bytes;

                evaluate(ast->left, bytes, n_bytes, data, current_stack_index, e);
                add_instruction(JMP_NOT);

                uint32_t placeholder_false_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_false_index);

                // true
                evaluate(ast->right->left, bytes, n_bytes, data, current_stack_index, e);

                uint32_t n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i)
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

                int32_t position;
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
                uint32_t n_parameters = 0;

                struct node* parameter = ast->left;
                while (parameter) {
                    add_variable(parameter->token->value, ast->scope + 1, -1 - n_parameters, e);
                    parameter = parameter->left;
                    ++n_parameters;
                }

                add_instruction(JMP);

                uint32_t placeholder_index = *n_bytes;
                (*n_bytes) += sizeof(placeholder_index);

                const char* fun_name = ast->token->value;
                add_function(fun_name, n_parameters, *n_bytes, e);

                // the first 2 are old base and return address
                uint32_t new_stack_index = 2;
                evaluate(ast->right, bytes, n_bytes, data, &new_stack_index,  e);

                uint32_t n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; i++) {
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

#define stack_base \
    vm->stack[STACK_BASE_INDEX]

#define stack_size \
    vm->stack[STACK_SIZE_INDEX]

#define return_value \
    vm->stack[RETURN_INDEX]

#define program_counter \
    vm->stack[PROGRAM_COUNTER]

#define read_value(value_type) \
    (*((value_type*)(vm->bytes + program_counter + 1)))

#define push(value) \
    vm->stack[stack_size++] = value

#define pop() \
    vm->stack[--stack_size]

int execute(struct vm* vm) {
    // 0 - stack size
    // 1 - stack base
    // 2 - return value
    // 3 - program counter

    stack_size = 4;
    stack_base = 4;
    program_counter = 0;

    while (!vm->halt && program_counter < vm->n_bytes) {
        switch (vm->bytes[program_counter]) {
            case PUSH:
                push(read_value(int32_t));
                program_counter += sizeof(uint32_t);
                break;

            case POP:
                --stack_size;
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
                    uint32_t number1 = pop();
                    uint32_t number2 = pop();
                    push(number1 * number2);
                }
                break;

            case DIV:
                {
                    uint32_t number1 = pop();
                    uint32_t number2 = pop();
                    push(number1 / number2);
                }
                break;

            case NOT:
                {
                    uint32_t exp = pop();
                    if (exp) {
                        push(0);
                    } else {
                        push(1);
                    }
                }
                break;

            case DEQ:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 == exp2) ? 1 : 0);
                }
                break;

            case NEQ:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 != exp2) ? 1 : 0);
                }
                break;

            case GRE:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 > exp2) ? 1 : 0);
                }
                break;

            case GRQ:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 >= exp2) ? 1 : 0);
                }
                break;

            case LES:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 < exp2) ? 1 : 0);
                }
                break;

            case LEQ:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 <= exp2) ? 1 : 0);
                }
                break;

            case AND:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 && exp2) ? 1 : 0);
                }
                break;

            case OR:
                {
                    uint32_t exp1 = pop();
                    uint32_t exp2 = pop();
                    push((exp1 || exp2) ? 1 : 0);
                }
                break;

            case PRINT:
                {
                    uint32_t number = pop();
                    printf("%d\n", number);
                }
                break;

            case DUP:
                {
                    int32_t index = read_value(int32_t);
                    push(vm->stack[stack_base + index]);
                    program_counter += sizeof(index);
                }
                break;
            case CHANGE:
                {
                    int32_t index = read_value(int32_t);
                    vm->stack[stack_base + index] = pop();
                    program_counter += sizeof(index);
                }
                break;
            case JMP_NOT:
                {
                    uint32_t condition = pop();
                    int32_t index = read_value(int32_t);

                    if (!condition) {
                        program_counter = index - 1;
                    } else {
                        program_counter += sizeof(index);
                    }
                }
                break;

            case JMP:
                {
                    int32_t index = read_value(int32_t);
                    program_counter = index - 1;
                }
                break;

            case CALL:
                {
                    uint32_t old_base = stack_base;
                    stack_base = stack_size;

                    push(program_counter + 1 + sizeof(int32_t));

                    int32_t index = read_value(int32_t);
                    program_counter = index - 1;

                    push(old_base);
                }
                break;

            case RET:
                {
                    stack_base = pop();

                    uint32_t index = pop();
                    program_counter = index - 1;
                }
                break;
        }

        ++program_counter;
    }

    return 0;
}

