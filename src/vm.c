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

static void add_variable(const char* var_name, uint32_t scope, uint32_t index, struct evaluator* e) {
    e->locals[e->n_locals++] = (struct var) {
        .name = var_name,
        .scope = scope,
        .stack_index = index
    };
}

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

static struct class_* get_class(const char* class_name, struct evaluator* e) {
    for (uint i = 0; i < e->n_functions; ++i) {
        if (strcmp(e->classes[i].name, class_name) == 0) {
            return &e->classes[i];
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

    if (o->type == OBJ_CLASS) {
        *(struct object_class*)(&data->constants_bytes[data->n_constants_bytes]) = *o->class_value;
        data->n_constants_bytes += sizeof(struct object_class);

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

int add_builtin_functions(struct evaluator* e) {
    uint32_t index = 0;

    e->functions[e->n_functions++] = (struct function){.name = "print",        .n_parameters = 1, .fun = print_object, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "input_number", .n_parameters = 1, .fun = input_number, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "input_string", .n_parameters = 1, .fun = input_string, .index = index++};

    return 0;
};

struct list_context {
    struct object container[20];
    uint32_t n_elements;
};

static struct object list_constructor(struct object self, struct vm* vm) {
    (void)vm;

    struct list_context* context = malloc(sizeof(*context));
    *context = (struct list_context) {};

    self.instance_value->members[0] = (struct object) {
        .type = OBJ_USER,
        .user_value = context
    };

    return (struct object){};
}

static struct object list_add(struct object self, struct vm* vm) {
    struct list_context* context = self.instance_value->members[0].user_value;
    context->container[context->n_elements++] = *peek(0);

    return (struct object){};
}

static struct object list_pop(struct object self, struct vm* vm) {
    (void)vm;
    struct list_context* context = self.instance_value->members[0].user_value;

    return context->container[--context->n_elements];
}

static struct object list_set(struct object self, struct vm* vm) {
    struct list_context* context = self.instance_value->members[0].user_value;

    uint32_t index = peek(0)->int_value;
    context->container[index] = *peek(1);

    return (struct object){};
}

static struct object list_get(struct object self, struct vm* vm) {
    struct list_context* context = self.instance_value->members[0].user_value;

    uint32_t index = peek(0)->int_value;
    return context->container[index];
}

int add_builtin_classes(struct evaluator* e) {
    struct class_ list = {
        .name = "list",
        .index = 0,
        .n_methods = 5,
        .methods = {
            {
                .name = "constructor",
                .method = list_constructor
            },
            {
                .name = "add",
                .method = list_add
            },
            {
                .name = "pop",
                .method = list_pop
            },
            {
                .name = "__set",
                .method = list_set
            },
            {
                .name = "__get",
                .method = list_get
            }
        }
    };

    e->classes[e->n_classes++] = list;

    return 0;
}

static int evaluate_lvalue(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope) {
    switch (ast->type) {
        case NODE_VAR:
            {
                struct var* variable;
                int res = get_variable(ast->token->value, e, &variable);
                if (res == 0) {
                    add_instruction(PUSH_NUM);
                    add_number(variable->stack_index);

                    // variable outside of function
                    if (variable->scope < function_scope) {
                        add_instruction(CHANGE);
                    } else {
                        add_instruction(CHANGE_LOC);
                    }

                    return 0;
                }

                return 1;
            }
        case NODE_MEMBER_ACCESS:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate left member of member access");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = ast->token->value}, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_instruction(SET_FIELD);

                return 0;
            }
        case NODE_INDEX:
            {
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope, current_scope), "failed to evaluate left member of member index expression");

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate left member of member index");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = "__set"}, &out_address);
                add_instruction(PUSH)
                add_number(out_address);

                add_instruction(GET_FIELD);
                add_instruction(CALL);
                add_instruction(POP);
                add_instruction(POP);

                add_instruction(DUP_REG);
                add_number(RETURN_REGISTER);

                return 0;
            }
        default:
            return 1;
    }

    return 1;
}

static int evaluate_class(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope, struct object_class* current_class) {
    if (ast == NULL)
        return 0;

    switch (ast->type) {
        case NODE_METHOD_FUN:
            {
                current_class->methods[current_class->n_methods++] = (struct pair){
                    .name = ast->token->value,
                    .index = data->n_program_bytes + sizeof(int32_t) + 1
                };

                CHECK(evaluate(e, ast, data, current_stack_index, function_scope, current_scope), "failed to evaluate method");
                return 0;
            }
        case NODE_MEMBER:
            {
                CHECK(evaluate_class(e, ast->right, data, current_stack_index, function_scope, current_scope, current_class), "failed to evaluate next member");
                current_class->members[current_class->n_members++] = ast->token->value;

                return 0;
            }

        case NODE_METHOD:
            {
                CHECK(evaluate_class(e, ast->right, data, current_stack_index, function_scope, current_scope, current_class), "failed to evaluate next method");
                CHECK(evaluate_class(e, ast->left, data, current_stack_index, function_scope, current_scope, current_class), "failed to evaluate next method");

                return 0;
            }
        default:
            return 1;
    }

    return 1;
}

int evaluate(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope) {
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
                if (f) {
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

                struct class_* c = get_class(name, e);
                if (c) {
                    struct object o = {
                        .type = OBJ_CLASS,
                        .class_value = &(struct object_class) {
                            .type = BUILT_IN,
                            .index = c->index
                        }
                    };

                    int32_t out_address;
                    add_constant(data, &o, &out_address);

                    add_instruction(PUSH);
                    add_number(out_address);

                    return 0;
                }

                return 1;
            }
        case NODE_BINARY_OP:
            CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope, current_scope), "failed to evaluate binary operation");
            CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope),  "failed to evaluate binary operation");

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
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate expression in if statement");

                add_instruction(JMP_NOT);
                uint32_t placeholder_true = create_placeholder();

                // true
                CHECK(evaluate(e, ast->right->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate \"true\" side in if statement");

                uint32_t placeholder_false;
                if (ast->right->right) {
                    add_instruction(JMP);
                    placeholder_false = create_placeholder();
                }

                // fill placeholder
                patch_placeholder(placeholder_true);

                // false
                CHECK(evaluate(e, ast->right->right, data, current_stack_index, function_scope, current_scope), "failed to evaluate \"false\" side in if statement");

                if (ast->right->right) {
                    patch_placeholder(placeholder_false);
                }

                return 0;
            }

        case NODE_WHILE:
            {
                uint32_t start_index = data->n_program_bytes;

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate expression in while statement");

                add_instruction(JMP_NOT);
                uint32_t placeholder_false = create_placeholder();

                // body
                CHECK(evaluate(e, ast->right->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate while statement body");

                add_instruction(JMP);
                add_number(start_index);

                // fill placeholder
                patch_placeholder(placeholder_false);

                return 0;
            }

        case NODE_DECLARATION:
            {
                const char* var_name = ast->token->value;
                add_variable(var_name, current_scope, *current_stack_index, e);
                increment_index();

                if (ast->left == NULL) {
                    add_instruction(PUSH_FALSE);
                } else {
                    CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate initialization value");
                }

                return 0;
            }
        case NODE_ASSIGN:
            {
                // first evaluate value to assign
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope, current_scope), "failed to evaluate assignment value");

                // then evaluate the lvalue
                CHECK(evaluate_lvalue(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate lvalue");

                return 0;
            }
        case NODE_STATEMENT:
            {
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope, current_scope), "failed to evaluate next statement");
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate current statement");

                return 0;
            }

        case NODE_BLOCK:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope + 1), "failed to evaluate current statement");

                uint32_t n_cleaned = pop_variables(current_scope, e);
                for (uint32_t i = 0; i < n_cleaned; ++i) {
                    add_instruction(POP);
                }

                return 0;
            }
        case NODE_NOT:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate not statement");
                add_instruction(NOT);

                return 0;
            }

        case NODE_CLASS:
            {
                const char* class_name = ast->token->value;
                struct object cls = {
                    .type = OBJ_CLASS,
                    .class_value = &(struct object_class) {
                        .name = class_name
                    }
                };

                // evaluate members
                CHECK(evaluate_class(e, ast->left, data, current_stack_index, function_scope, current_scope, cls.class_value), "failed to evaluate class members");

                // evaluate methods
                CHECK(evaluate_class(e, ast->right, data, current_stack_index, function_scope, current_scope, cls.class_value), "failed to evaluate class methods");

                int32_t out_address;
                add_constant(data, &cls, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_variable(class_name, current_scope, *current_stack_index, e);
                increment_index();

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

                add_variable(fun_name, current_scope, *current_stack_index, e);
                increment_index();

                parameter = ast->left;
                while (parameter) {
                    --n_parameters;
                    add_variable(parameter->token->value, current_scope + 1, -1 - n_parameters, e);
                    parameter = parameter->right;
                }

                // the first 2 are old base and return address
                uint32_t new_stack_index = 2;
                CHECK(evaluate(e, ast->right, data, &new_stack_index, function_scope + 1, current_scope), "failed to evaluate function body");

                // TODO: double ret in case of return
                add_instruction(RET);

                patch_placeholder(placeholder);

                return 0;
            }

        case NODE_METHOD_FUN:
            {
                uint32_t n_parameters = 0;

                struct node* parameter = ast->left;
                while (parameter) {
                    parameter = parameter->right;
                    ++n_parameters;
                }
                ++n_parameters;

                add_instruction(JMP);
                uint32_t placeholder = create_placeholder();

                parameter = ast->left;
                while (parameter) {
                    --n_parameters;
                    add_variable(parameter->token->value, current_scope + 1, -1 - n_parameters, e);
                    parameter = parameter->right;
                }

                --n_parameters;
                add_variable("self", current_scope + 1, -1 - n_parameters, e);

                // the first 2 are old base and return address
                uint32_t new_stack_index = 2;
                CHECK(evaluate(e, ast->right, data, &new_stack_index, function_scope + 1, current_scope), "failed to evaluate function body");

                if (strcmp(ast->token->value, "constructor") == 0) {
                    add_instruction(RET_INS);
                } else {
                    add_instruction(RET);
                }

                patch_placeholder(placeholder);

                return 0;
            }

        case NODE_CALL:
            {
                uint32_t n_arguments = 0;
                struct node* argument = ast->right;

                while (argument) {
                    CHECK(evaluate(e, argument->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate function argument");
                    argument = argument->right;
                    ++n_arguments;
                }

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate lvalue to call");
                add_instruction(CALL);

                for (uint32_t i = 0; i < n_arguments; ++i) {
                    add_instruction(POP);
                }

                add_instruction(DUP_REG);
                add_number(RETURN_REGISTER);

                return 0;
            }
        case NODE_MEMBER_ACCESS:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate left member of member access");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = ast->token->value}, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_instruction(GET_FIELD);

                return 0;
            }
        case NODE_INDEX:
            {
                CHECK(evaluate(e, ast->right, data, current_stack_index, function_scope, current_scope), "failed to evaluate left member of member index expression");

                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate left member of member index");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = "__get"}, &out_address);
                add_instruction(PUSH)
                add_number(out_address);

                add_instruction(GET_FIELD);
                add_instruction(CALL);
                add_instruction(POP);
                add_instruction(DUP_REG);
                add_number(RETURN_REGISTER);

                return 0;
            }
        case NODE_RETURN:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate return value");

                add_instruction(CHANGE_REG);
                add_number(RETURN_REGISTER);

                add_instruction(RET);

                return 0;
            }
        case NODE_EXP_STATEMENT:
            {
                CHECK(evaluate(e, ast->left, data, current_stack_index, function_scope, current_scope), "failed to evaluate return value");
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

static void push_method(struct vm* vm, struct object_method* method) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_METHOD, .method_value = method};
}

static void push_class(struct vm* vm, struct object_class* cls) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_CLASS, .class_value = cls};
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
        return;
    }

    if (type == OBJ_CLASS) {
        struct object_class* cls = (struct object_class*)&vm->bytes[address];
        push_class(vm, cls);
        return;
    }
}

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
*/

static int pop_string_check(struct vm* vm, const char** out_string) {
    struct object* obj = pop();
    if (obj->type != OBJ_STRING) {
        ERROR("string required, got %d", obj->type);
        return 1;
    }

    *out_string = obj->str_value;
    return 0;
}

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

static void call(struct vm* vm, int32_t index) {
    uint32_t old_base = vm->stack_base;
    vm->stack_base = vm->stack_size;

    push_number(vm, vm->program_counter + 1);
    push_number(vm, old_base);

    vm->program_counter = vm->start_address + index - 1;
}

static void return_(struct vm* vm) {
    vm->stack_size = vm->stack_base + 2;
    vm->stack_base = pop_number(vm);
    int32_t index = pop_number(vm);

    vm->program_counter = index - 1;
}

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
    vm->program_counter = vm->start_address;

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
                        vm->program_counter = vm->start_address + index - 1;
                    }
                }
                break;

            case JMP:
                {
                    int32_t index = read_value(int32_t);
                    vm->program_counter = vm->start_address + index - 1;
                }
                break;

            case CALL:
                {
                    struct object o = *pop();

                    if (o.type == OBJ_CLASS) {
                        struct object_class* cls = o.class_value;

                        struct object_instance* value = malloc(sizeof(*value));
                        if (value == NULL)
                            return 1;

                        if (cls->type == USER) {
                            *value = (struct object_instance) {
                                .type = USER,
                                .class_index = cls
                            };
                        } else {
                            *value = (struct object_instance) {
                                .type = BUILT_IN,
                                .buintin_index = &vm->builtin_classes[cls->index]
                            };
                        }

                        struct object o = (struct object) {
                            .type = OBJ_INSTANCE,
                            .instance_value = value
                        };

                        if (cls->type == BUILT_IN) {
                            vm->builtin_classes[cls->index].methods[0].method(o, vm);
                        } else {
                            uint32_t i;
                            for (i = 0; i < cls->n_methods; ++i) {
                                // TODO: this don't need to have return
                                if (strcmp(cls->methods[i].name, "constructor") == 0) {
                                    push(o);
                                    call(vm, cls->methods[i].index);
                                    break;
                                }
                            }

                            if (i == cls->n_methods)
                                return 1;
                        }

                        vm->registers[RETURN_REGISTER] = o;
                        break;
                    }

                    if (o.type == OBJ_METHOD) {
                        if (o.method_value->type == USER) {
                            push(o.method_value->context);
                            call(vm, o.method_value->index);
                        } else if (o.method_value->type == BUILT_IN) {
                            struct object* instance = &o.method_value->context;
                            struct class_* cls = instance->instance_value->buintin_index;
                            struct method* method = &cls->methods[o.method_value->index];

                            vm->registers[RETURN_REGISTER] = method->method(*instance, vm);
                        }
                        break;
                    }

                    if (o.type == OBJ_FUNCTION) {
                        if (o.function_value->type == USER) {
                            call(vm, o.function_value->index);
                        } else if (o.function_value->type == BUILT_IN) {
                            vm->registers[RETURN_REGISTER] = vm->builtin_functions[o.function_value->index].fun(vm);
                        }
                    }
                }
                break;

            case GET_FIELD:
                {
                    const char* field_name;
                    CHECK(pop_string_check(vm, &field_name), "field name is not a string");

                    struct object instance = *pop();

                    if (instance.instance_value->type == BUILT_IN) {
                        struct class_* cls = instance.instance_value->buintin_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                push(instance.instance_value->members[i]);
                                break;
                            }
                        }

                        if (i == cls->n_members) {
                            for (i = 0; i < cls->n_methods; ++i) {
                                if (strcmp(cls->methods[i].name, field_name) == 0) {
                                    push_method(vm, &(struct object_method){.type = BUILT_IN, .index = i, .n_parameters = 0, .context = instance});
                                    break;
                                }
                            }
                        }

                        if (i == cls->n_methods)
                            return 1;

                        break;
                    }

                    struct object_class* cls = instance.instance_value->class_index;
                    uint32_t i = 0;
                    for (i = 0; i < cls->n_members; ++i) {
                        if (strcmp(cls->members[i], field_name) == 0) {
                            push(instance.instance_value->members[i]);
                            break;
                        }
                    }

                    if (i == cls->n_members) {
                        for (i = 0; i < cls->n_methods; ++i) {
                            if (strcmp(cls->methods[i].name, field_name) == 0) {
                                push_method(vm, &(struct object_method){.type = USER, .index = cls->methods[i].index, .n_parameters = cls->methods[i].n_parameters, .context = instance});
                                break;
                            }
                        }
                    }

                    if (i == cls->n_methods)
                        return 1;
                }
                break;

            case SET_FIELD:
                {
                    const char* field_name;
                    CHECK(pop_string_check(vm, &field_name), "field name is not a string");

                    struct object* instance = pop();

                    if (instance->instance_value->type == BUILT_IN) {
                        struct class_* cls = instance->instance_value->buintin_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                instance->instance_value->members[i] = *pop();
                                break;
                            }
                        }

                        if (i == cls->n_members)
                            return 1;

                        break;
                    }

                    struct object_class* cls = instance->instance_value->class_index;
                    uint32_t i;
                    for (i = 0; i < cls->n_members; ++i) {
                        if (strcmp(cls->members[i], field_name) == 0) {
                            instance->instance_value->members[i] = *pop();
                            break;
                        }
                    }

                    if (i == cls->n_members)
                        return 1;
                }
                break;
            case RET:
                {
                    return_(vm);
                }
                break;
            case RET_INS:
                {
                    vm->registers[RETURN_REGISTER] = vm->stack[vm->stack_base - 1];
                    return_(vm);
                    --vm->stack_size;
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

