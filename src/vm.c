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

static int get_variable(const char* name, struct evaluator* e, struct var** out_var) {
    uint32_t i = e->n_locals;

    while (i > 0) {
        --i;

        if (strcmp(name, e->locals[i].name) == 0) {
            *out_var = &e->locals[i];
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

static uint32_t get_var_count(uint32_t scope, struct evaluator* e) {
    uint32_t count = 0;
    uint32_t n = e->n_locals;

    while (n > 0 && e->locals[n - 1].scope > scope) {
        if (e->locals[n - 1].stack_index >= 0)
            count += 1;

        --n;
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

static int32_t add_constant(struct binary_data* data, const struct object* o, int32_t* out_address) {
    uint32_t constant_address = data->n_constants_bytes;

    *(int32_t*)(&data->constants_bytes[data->n_constants_bytes]) = o->type;
    data->n_constants_bytes += sizeof(int32_t);

    if (o->type == OBJ_NUMBER) {
        *(int32_t*)(&data->constants_bytes[data->n_constants_bytes]) = o->int_value;
        data->n_constants_bytes += sizeof(int32_t);

        *out_address = constant_address;
        return 0;
    }

    if (o->type == OBJ_STRING) {
        strcpy((char*)&data->constants_bytes[data->n_constants_bytes], o->str_value);
        data->n_constants_bytes += strlen(o->str_value);

        data->constants_bytes[data->n_constants_bytes] = '\0';
        ++data->n_constants_bytes;

        *out_address = constant_address;
        return 0;
    }

    if (o->type == OBJ_FUNCTION) {
        *(struct object_function*)(&data->constants_bytes[data->n_constants_bytes]) = *o->function_value;
        data->n_constants_bytes += sizeof(struct object_function);

        *out_address = constant_address;
        return 0;
    }

    return 1;
}

#define RETURN_REGISTER 0

#define add_instruction(code) \
{ \
    data->program_bytes[(data->n_program_bytes)++] = code; \
}

#define add_number(num) \
{ \
    int32_t int_value = (int32_t)(num); \
    memcpy(&data->program_bytes[data->n_program_bytes], &int_value, sizeof(int_value)); \
    data->n_program_bytes += sizeof(num); \
}

#define increment_index() \
    ++(*current_stack_index)

#define create_placeholder() \
    data->n_program_bytes; (data->n_program_bytes) += sizeof(uint32_t)

#define patch_placeholder(placeholder) \
    { \
        memcpy(&data->program_bytes[placeholder], &data->n_program_bytes, sizeof(data->n_program_bytes)); \
    } \

int add_builtin_functions(struct evaluator* e) {
    e->functions[e->n_functions++] = (struct function){.type = BUILT_IN, .name = "print",        .n_parameters = 1, .index = 0};
    e->functions[e->n_functions++] = (struct function){.type = BUILT_IN, .name = "input_number", .n_parameters = 1, .index = 1};
    e->functions[e->n_functions++] = (struct function){.type = BUILT_IN, .name = "input_string", .n_parameters = 1, .index = 2};

    return 0;
};


static int evaluate_lvalue(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* out_scope) {
    switch (ast->type) {
        case NODE_VAR:
            {
                struct var* variable;
                int res = get_variable(ast->token->value, e, &variable);
                if (res == 0) {
                    add_instruction(PUSH_NUM);
                    add_number(variable->stack_index);
                    *out_scope = variable->scope;

                    return 0;
                }
            }
    }

    return 1;
}

int evaluate(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope) {
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_NUMBER:
            {
                add_instruction(PUSH);

                int32_t constant_address;
                CHECK(add_constant(data, &(struct object){.type = OBJ_NUMBER, .int_value = *(int32_t*)ast->token->value}, &constant_address), "failed to add constant");

                add_number(constant_address);

                return 0;
            }
        case NODE_BOOL:
            {
                if (ast->token->code == TOK_TRU) {
                    add_instruction(PUSH_TRUE);
                } else {
                    add_instruction(PUSH_FALSE);
                }

                return 0;
            }
        case NODE_STRING:
            {
                add_instruction(PUSH);

                int32_t constant_address;
                CHECK(add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = ast->token->value}, &constant_address), "failed to add constant");

                add_number(constant_address);

                return 0;
            }
        case NODE_VAR:
            {
                struct var* variable;
                int res = get_variable(ast->token->value, e, &variable);
                if (res == 0) {
                    // variable outside of function
                    if (variable->scope < function_scope) {
                        add_instruction(DUP);
                    } else {
                        add_instruction(DUP_LOC);
                    }

                    add_number(variable->stack_index);

                    return 0;
                }

                const char* name = ast->token->value;
                struct function* f = get_function(name, e);
                if (f == NULL) {
                    ERROR("identifier %s does not exist", name);
                    return 1;
                }

                struct object o = {
                    .type = OBJ_FUNCTION,
                    .function_value = &(struct object_function) {
                        .type = BUILT_IN,
                        .n_parameters = f->n_parameters,
                        .index = f->index
                    }
                };

                int32_t out_address;
                add_constant(data, &o, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                return 0;
            }
        case NODE_BINARY_OP:
            CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope), "failed to evaluate binary operation");
            CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope),  "failed to evaluate binary operation");

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
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate expression in if statement");

                add_instruction(JMP_NOT);
                uint32_t placeholder_true = create_placeholder();

                // true
                CHECK(evaluate(e, ast->right->left, data, current_stack_index, function_scope), "failed to evaluate \"true\" side in if statement");

                uint32_t n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i) {
                    add_instruction(POP);
                }

                uint32_t placeholder_false;
                if (ast->right->right) {
                    add_instruction(JMP);
                    placeholder_false = create_placeholder();
                }

                // fill placeholder
                patch_placeholder(placeholder_true);

                // false
                CHECK(evaluate(e, ast->right->right, data, current_stack_index, function_scope), "failed to evaluate \"false\" side in if statement");

                if (ast->right->right) {
                    n_cleaned = pop_variables(ast->scope, e);
                    for (uint32_t i = 0; i < n_cleaned; ++i) {
                        add_instruction(POP);
                    }

                    patch_placeholder(placeholder_false);
                }

                return 0;
            }

        case NODE_WHILE:
            {
                uint32_t start_index = data->n_program_bytes;

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate expression in while statement");

                add_instruction(JMP_NOT);
                uint32_t placeholder_false = create_placeholder();

                // body
                CHECK(evaluate(e, ast->right->left, data, current_stack_index, function_scope), "failed to evaluate while statement body");

                uint32_t n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i) {
                    add_instruction(POP);
                }

                add_instruction(JMP);
                add_number(start_index);

                // fill placeholder
                patch_placeholder(placeholder_false);

                return 0;
            }

        case NODE_DECLARATION:
            {
                const char* var_name = ast->token->value;
                add_variable(var_name, ast->scope, *current_stack_index, e);
                increment_index();

                if (ast->left == NULL) {
                    add_instruction(PUSH_FALSE);
                } else {
                    CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate initialization value");
                }

                return 0;
            }
        case NODE_ASSIGN:
            {
                // first evaluate value to assign
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope), "failed to evaluate assignment value");

                // then evaluate the lvalue
                uint32_t lvalue_scope;
                CHECK(evaluate_lvalue(e, ast->left, data, &lvalue_scope), "failed to evaluate lvalue");

                // variable outside of function
                if (lvalue_scope < function_scope) {
                    add_instruction(CHANGE);
                } else {
                    add_instruction(CHANGE_LOC);
                }

                return 0;
            }
        case NODE_STATEMENT:
            {
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope), "failed to evaluate next statement");
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate current statement");

                return 0;
            }

        case NODE_NOT:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate not statement");
                add_instruction(NOT);

                return 0;
            }

        case NODE_CLASS:
            {
                const char* class_name = ast->token->value;
                e->classes[e->n_classes++] = (struct object_class) {
                    .name = class_name
                };

                // evaluate members
                evaluate(e, ast->left, data, current_stack_index, function_scope);

                // evaluate methods
                evaluate(e, ast->right, data, current_stack_index, function_scope);

                return 0;
            }

        case NODE_MEMBER:
            {
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope), "failed to evaluate next member");

                struct object_class* current_class = &e->classes[e->n_classes - 1];
                current_class->members[current_class->n_members++] = (struct pair){
                    ast->left->token->value,
                    data->n_program_bytes
                };

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate member");

                return 0;
            }

        case NODE_METHOD:
            {
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope), "failed to evaluate next method");

                struct object_class* current_class = &e->classes[e->n_classes - 1];
                current_class->methods[current_class->n_methods++] = (struct pair){
                    ast->left->token->value,
                    data->n_program_bytes
                };

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate method");

                return 0;
            }
        case NODE_FUNCTION:
            {
                uint32_t n_parameters = 0;

                struct node* parameter = ast->left;
                while (parameter) {
                    parameter = parameter->right;
                    ++n_parameters;
                }

                const char* fun_name = ast->token->value;
                struct object o = {
                    .type = OBJ_FUNCTION,
                    .function_value = &(struct object_function) {
                        .type = USER,
                        .n_parameters = n_parameters,
                        .index = data->n_program_bytes + sizeof(int32_t) + sizeof(int32_t) + 2
                   }
                };

                int32_t out_address;
                add_constant(data, &o, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_instruction(JMP);
                uint32_t placeholder = create_placeholder();

                add_variable(fun_name, ast->scope, *current_stack_index, e);
                increment_index();

                parameter = ast->left;
                while (parameter) {
                    --n_parameters;
                    add_variable(parameter->token->value, ast->scope + 1, -1 - n_parameters, e);
                    parameter = parameter->right;
                }

                // the first 2 are old base and return address
                uint32_t new_stack_index = 2;
                CHECK(evaluate(e, ast->right, data, &new_stack_index, function_scope + 1), "failed to evaluate function body");

                uint32_t n_cleaned = pop_variables(ast->scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i) {
                    add_instruction(POP);
                }

                // TODO: double ret in case of return
                add_instruction(RET);

                patch_placeholder(placeholder);

                return 0;
            }

        case NODE_CALL:
            {
                uint32_t n_arguments = 0;
                struct node* argument = ast->right;

                while (argument) {
                    CHECK(evaluate(e, argument->left, data, current_stack_index, function_scope), "failed to evaluate function argument");
                    argument = argument->right;
                    ++n_arguments;
                }

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate lvalue to call");
                add_instruction(CALL);

                for (uint32_t i = 0; i < n_arguments; ++i) {
                    add_instruction(POP);
                }

                add_instruction(DUP_REG);
                add_number(RETURN_REGISTER);

                return 0;
            }

        case NODE_RETURN:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate return value");

                add_instruction(CHANGE_REG);
                add_number(RETURN_REGISTER);

                uint32_t n_cleaned = get_var_count(0, e);
                for (uint32_t i = 0; i < n_cleaned; ++i) {
                    add_instruction(POP);
                }

                add_instruction(RET);

                return 0;
            }
        case NODE_EXP_STATEMENT:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope), "failed to evaluate return value");
                add_instruction(POP);

                return 0;
            }

        default:
            return 1;
    }

    return 0;
}

#define read_value(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1)))

#define read_value_increment(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1))); vm->program_counter += sizeof(value_type)


static void push_number(struct vm* vm, int32_t number) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_NUMBER, .int_value = number};
}

static void push_string(struct vm* vm, char* string) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_STRING, .str_value = string};
}

static void push_bool(struct vm* vm, bool value) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_BOOL, .bool_value = value};
}

static void push_func(struct vm* vm, struct object_function* func) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_FUNCTION, .function_value = func};
}

static void push_constant(struct vm* vm, int32_t address) {
    int type = *((int32_t*)&vm->bytes[address]);
    address += sizeof(int32_t);

    void* value;
    if (type == OBJ_NUMBER) {
        int32_t number = *((int32_t*)&vm->bytes[address]);
        push_number(vm, number);
        return;
    }

    if (type == OBJ_STRING) {
        value = &vm->bytes[address];
        push_string(vm, value);
        return;
    }

    if (type == OBJ_FUNCTION) {
        struct object_function* func = (struct object_function*)&vm->bytes[address];
        push_func(vm, func);
    }
}

#define push(o) \
    vm->stack[vm->stack_size++] = o

#define pop() \
    (&vm->stack[--vm->stack_size])

#define peek(n) \
    (&vm->stack[vm->stack_size - 1 - n])

static int32_t pop_number(struct vm* vm) {
    struct object* obj = pop();
    return obj->int_value;
}

static int pop_number_check(struct vm* vm, int32_t* out_number) {
    struct object* obj = pop();
    if (obj->type != OBJ_NUMBER) {
        ERROR("number required, got %d", obj->type);
        return 1;
    }

    *out_number = obj->int_value;
    return 0;
}

/*
// unused for the moment
static const char* pop_string(struct vm* vm) {
    struct object* obj = pop();
    return obj->str_value;
}

static int pop_string_check(struct vm* vm, const char** out_string) {
    struct object* obj = pop();
    if (obj->type != OBJ_STRING) {
        ERROR("string required, got %d", obj->type);
        return 1;
    }

    *out_string = obj->str_value;
    return 0;
}
*/

bool pop_bool(struct vm* vm) {
    struct object* obj = pop();
    return obj->bool_value;
}

bool pop_bool_check(struct vm* vm, bool* out_bool) {
    struct object* obj = pop();
    if (obj->type != OBJ_BOOL) {
        ERROR("bool required, got %d", obj->type);
        return 1;
    }

    *out_bool = obj->bool_value;
    return 0;
}

typedef struct object (*builtin_fun)(struct vm* vm);

static struct object print_object(struct vm* vm) {
    struct object* o = peek(0);

    if (o->type == OBJ_NUMBER) {
        printf("%d\n", o->int_value);
    } else if (o->type == OBJ_STRING) {
        printf("%s\n", o->str_value);
    }

    return (struct object){};
}

static struct object input_number(struct vm* vm) {
    struct object* o = (struct object*)peek(0);

    const char* input_text = o->str_value;
    printf("%s", input_text);

    int32_t number;
    scanf("%d", &number);

    puts("");

    return (struct object){.type = OBJ_NUMBER, .int_value = number};
};

static struct object input_string(struct vm* vm) {
    struct object* o = (struct object*)peek(0);

    const char* input_text = o->str_value;
    printf("%s", input_text);

    char* string = malloc(200);
    scanf("%s", string);

    puts("");

    return (struct object){.type = OBJ_STRING, .str_value = string};
}

static builtin_fun builtin_functions[] = {
    print_object,
    input_number,
    input_string
};

static int add_strings(struct vm* vm, struct object* value1, struct object* value2) {
    char* new_string = malloc(500);
    uint32_t n_new_string = 0;

    if (value1->type == OBJ_NUMBER) {
        n_new_string += sprintf(new_string, "%d", value1->int_value);
    } else if (value1->type == OBJ_STRING) {
        strcpy(new_string, value1->str_value);
        n_new_string += strlen(value1->str_value);
    } else {
        return 1;
    }

    if (value2->type == OBJ_NUMBER) {
        n_new_string += sprintf(new_string + n_new_string, "%d", value2->int_value);
    } else if (value2->type == OBJ_STRING) {
        strcpy(new_string + n_new_string, value2->str_value);
        n_new_string += strlen(value2->str_value);
    } else {
        return 1;
    }

    push_string(vm, new_string);
    return 0;
}

int execute(struct vm* vm) {
    int32_t start_address = *(int32_t*)vm->bytes;
    vm->program_counter = start_address;

    while (!vm->halt && vm->program_counter < vm->n_bytes) {
        switch (vm->bytes[vm->program_counter]) {
            case PUSH:
                {
                    int32_t constant = read_value_increment(int32_t);
                    push_constant(vm, constant);
                }
                break;

            case PUSH_NUM:
                {
                    int32_t number = read_value_increment(int32_t);
                    push_number(vm, number);
                }
                break;

            case PUSH_TRUE:
                {
                    push_bool(vm, true);
                }
                break;

            case PUSH_FALSE:
                {
                    push_bool(vm, false);
                }
                break;

            case POP:
                --vm->stack_size;
                break;

            case ADD:
                {
                    struct object* value1 = pop();
                    struct object* value2 = pop();

                    if (value1->type == OBJ_STRING || value2->type == OBJ_STRING) {
                        CHECK(add_strings(vm, value1, value2), "failed to add objects");
                        break;
                    }

                    if (value1->type == OBJ_NUMBER && value2->type == OBJ_NUMBER) {
                        int32_t number1 = value1->int_value;
                        int32_t number2 = value2->int_value;

                        push_number(vm, number1 + number2);
                        break;
                    }

                    ERROR("add operation for object types: %d and %d not defined", value1->type, value2->type);
                    return 1;

                }
                break;

            case MIN:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for minus operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for minus operation is not a number");

                    push_number(vm, number1 - number2);
                }
                break;

            case MUL:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for multiply operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for multiply operation is not a number");

                    push_number(vm, number1 * number2);
                }
                break;

            case DIV:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for division operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for division operation is not a number");

                    push_number(vm, number1 / number2);
                }
                break;

            case NOT:
                {
                    bool exp;
                    CHECK(pop_bool_check(vm, &exp), "operand for not operation is not bool");

                    push_bool(vm, !exp);
                }
                break;

            case DEQ:
                {
                    struct object* exp1 = pop();
                    struct object* exp2 = pop();

                    if (exp1->type == OBJ_NUMBER && exp2->type == OBJ_NUMBER) {
                        push_bool(vm, exp1->int_value == exp2->int_value);
                        break;
                    }

                    if (exp1->type == OBJ_BOOL && exp2->type == OBJ_BOOL) {
                        push_bool(vm, exp1->bool_value == exp2->bool_value);
                        break;
                    }

                    push_bool(vm, false);
                }
                break;

            case NEQ:
                {
                    struct object* exp1 = pop();
                    struct object* exp2 = pop();

                    if (exp1->type == OBJ_NUMBER && exp2->type == OBJ_NUMBER) {
                        push_bool(vm, exp1->int_value != exp2->int_value);
                        break;
                    }

                    if (exp1->type == OBJ_BOOL && exp2->type == OBJ_BOOL) {
                        push_bool(vm, exp1->bool_value != exp2->bool_value);
                        break;
                    }

                    push_bool(vm, true);
                }
                break;

            case GRE:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for greater operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for greater operation is not a number");

                    push_bool(vm, number1 > number2);
                }
                break;

            case GRQ:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for greater or equal operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for greater or equal operation is not a number");

                    push_bool(vm, number1 >= number2);
                }
                break;

            case LES:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for less operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for less operation is not a number");

                    push_bool(vm, number1 < number2);
                }
                break;

            case LEQ:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for less or equal operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for less or equal operation is not a number");

                    push_bool(vm, number1 <= number2);
                }
                break;

            case AND:
                {
                    bool exp1;
                    CHECK(pop_bool_check(vm, &exp1), "operand 1 for and operation is not a number");

                    bool exp2;
                    CHECK(pop_bool_check(vm, &exp2), "operand 2 for and operation is not a number");

                    push_bool(vm, exp1 && exp2);
                }
                break;

            case OR:
                {
                    bool exp1;
                    CHECK(pop_bool_check(vm, &exp1), "operand 1 for or operation is not a number");

                    bool exp2;
                    CHECK(pop_bool_check(vm, &exp2), "operand 2 for or operation is not a number");

                    push_bool(vm, exp1 || exp2);
                }
                break;

            case DUP:
                {
                    int32_t index = read_value_increment(int32_t);
                    push(vm->stack[index]);
                }
                break;

            case DUP_LOC:
                {
                    int32_t index = read_value_increment(int32_t);
                    push(vm->stack[vm->stack_base + index]);
                }
                break;
            case DUP_REG:
                {
                    int32_t index = read_value_increment(int32_t);
                    push(vm->registers[index]);
                }
                break;
            case CHANGE:
                {
                    int32_t index = pop_number(vm);
                    vm->stack[index] = *pop();
                }
                break;
            case CHANGE_LOC:
                {
                    int32_t index = pop_number(vm);
                    vm->stack[vm->stack_base + index] = *pop();
                }
                break;
            case CHANGE_REG:
                {
                    int32_t index = read_value_increment(int32_t);
                    vm->registers[index] = *pop();
                }
                break;
            case JMP_NOT:
                {
                    int32_t index = read_value_increment(int32_t);

                    bool condition;
                    CHECK(pop_bool_check(vm, &condition), "operand for jump operation is not bool");

                    if (!condition) {
                        vm->program_counter = start_address + index - 1;
                    }
                }
                break;

            case JMP:
                {
                    int32_t index = read_value(int32_t);
                    vm->program_counter = start_address + index - 1;
                }
                break;

            case CALL:
                {
                    struct object o = *pop();

                    if (o.type == OBJ_FUNCTION) {
                        if (o.function_value->type == USER) {
                            uint32_t old_base = vm->stack_base;
                            vm->stack_base = vm->stack_size;

                            push_number(vm, vm->program_counter + 1);
                            push_number(vm, old_base);

                            vm->program_counter = start_address + o.function_value->index - 1;
                        } else if (o.function_value->type == BUILT_IN) {
                            vm->registers[RETURN_REGISTER] = builtin_functions[o.function_value->index](vm);
                        }
                    }
                }
                break;
            case RET:
                {
                    vm->stack_base = pop_number(vm);
                    int32_t index = pop_number(vm);

                    vm->program_counter = index - 1;
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

