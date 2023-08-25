#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include "parser.h"
#include "compiler.h"
#include "objects.h"
#include "instructions.h"
#include "sylk_lib.h"
#include "utils.h"

struct var {
    const char* name;
    void* value;
    uint32_t scope;
    int32_t stack_index;
    bool constant;
};

struct compiler_data {
    struct named_function* functions;
    uint32_t n_functions;

    struct named_class* classes;
    uint32_t n_classes;

    struct var locals[1024];
    uint32_t n_locals;
};

struct binary_data {
    uint8_t constants_bytes[4048];
    uint32_t n_constants_bytes;

    uint8_t classes[4048];
    uint32_t n_classes;

    uint8_t program_bytes[4048];
    uint32_t n_program_bytes;
};

static void add_variable(const char* var_name, uint32_t scope, uint32_t index, bool constant, struct compiler_data* e) {
    e->locals[e->n_locals++] = (struct var) {
        .name = var_name,
        .scope = scope,
        .stack_index = index,
        .constant = constant
    };
}

static int get_variable(const char* name, struct compiler_data* e, struct var** out_var) {
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

static uint32_t pop_variables(uint32_t scope, struct compiler_data* e) {
    uint32_t count = 0;

    while (e->n_locals > 0 && e->locals[e->n_locals - 1].scope > scope) {
        if (e->locals[e->n_locals - 1].stack_index >= 0)
            count += 1;

        --(e->n_locals);
    }

    return count;
}

static struct named_function* get_function(const char* function_name, struct compiler_data* e) {
    for (uint i = 0; i < e->n_functions; ++i) {
        if (strcmp(e->functions[i].name, function_name) == 0) {
            return &e->functions[i];
        }
    }

    return NULL;
}

static struct named_class* get_class(const char* class_name, struct compiler_data* e) {
    for (uint i = 0; i < e->n_classes; ++i) {
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
        *(int32_t*)(&data->constants_bytes[data->n_constants_bytes]) = o->num_value;
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
        *(struct object_function*)(&data->constants_bytes[data->n_constants_bytes]) = *(struct object_function*)o->obj_value;
        data->n_constants_bytes += sizeof(struct object_function);

        *out_address = constant_address;
        return 0;
    }

    if (o->type == OBJ_CLASS) {
        *(struct object_class*)(&data->constants_bytes[data->n_constants_bytes]) = *(struct object_class*)o->obj_value;
        data->n_constants_bytes += sizeof(struct object_class);

        *out_address = constant_address;
        return 0;
    }

    return 1;
}


#define add_instruction(code) \
{ \
    data->program_bytes[(data->n_program_bytes)++] = code; \
}

#define add_number(num) \
{ \
    int32_t int_value = (int32_t)(num); \
    memcpy(&data->program_bytes[data->n_program_bytes], &int_value, sizeof(int_value)); \
    data->n_program_bytes += sizeof(int_value); \
}

#define increment_index() \
    ++(*current_stack_index)

#define create_placeholder() \
    data->n_program_bytes; (data->n_program_bytes) += sizeof(uint32_t)

#define patch_placeholder(placeholder) \
    { \
        memcpy(&data->program_bytes[placeholder], &data->n_program_bytes, sizeof(data->n_program_bytes)); \
    } \


int compile(struct compiler_data* cd, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope, void* ctx);
static int compile_lvalue(struct compiler_data* cd, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope, void* ctx) {
    switch (ast->type) {
        case NODE_VAR:
            {
                struct var* variable;
                int res = get_variable(ast->token.value, cd, &variable);
                if (res == 0) {
                    if (variable->constant) {
                        ERROR("can't change a constant");
                        return 1;
                    }
                    // variable outside of function
                    if (variable->scope < function_scope) {
                        add_instruction(CHANGE);
                    } else {
                        add_instruction(CHANGE_LOC);
                    }

                    add_number(variable->stack_index);

                    return 0;
                }

                return 1;
            }
        case NODE_MEMBER_ACCESS:
            {
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile left member of member access");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = ast->token.value}, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_instruction(SET_FIELD);

                return 0;
            }
        case NODE_INDEX:
            {
                CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile left member of member index expression");

                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile left member of member index");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = "__set"}, &out_address);
                add_instruction(PUSH)
                add_number(out_address);

                add_instruction(GET_FIELD);

                add_instruction(CALL);
                add_number(2);

                return 0;
            }
        default:
            return 1;
    }

    return 1;
}

int compile(struct compiler_data* cd, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope, void* ctx) {
    if (ast == NULL) {
        return 0;
    }

    switch (ast->type) {
        case NODE_NUMBER:
            {
                add_instruction(PUSH);

                int32_t constant_address;
                CHECK(add_constant(data, &(struct object){.type = OBJ_NUMBER, .num_value = *(int32_t*)ast->token.value}, &constant_address), "failed to add constant");

                add_number(constant_address);

                return 0;
            }
        case NODE_BOOL:
            {
                if (ast->token.code == TOK_TRU) {
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
                CHECK(add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = ast->token.value}, &constant_address), "failed to add constant");

                add_number(constant_address);

                return 0;
            }
        case NODE_VAR:
            {
                struct var* variable;
                int res = get_variable(ast->token.value, cd, &variable);
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

                const char* name = ast->token.value;
                struct named_function* f = get_function(name, cd);
                if (f) {
                    struct object o = {
                        .type = OBJ_FUNCTION,
                        .obj_value = &f->function
                    };

                    int32_t out_address;
                    add_constant(data, &o, &out_address);

                    add_instruction(PUSH);
                    add_number(out_address);

                    return 0;
                }

                struct named_class* c = get_class(name, cd);
                if (c) {
                    struct object o = {
                        .type = OBJ_CLASS,
                        .obj_value = &c->cls
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
            CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile binary operation");
            CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx),  "failed to compile binary operation");

            switch (ast->token.code) {
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
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile expression in if statement");

                add_instruction(JMP_NOT);
                uint32_t placeholder_true = create_placeholder();

                // true
                CHECK(compile(cd, ast->right->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile \"true\" side in if statement");

                uint32_t placeholder_false;
                if (ast->right->right) {
                    add_instruction(JMP);
                    placeholder_false = create_placeholder();
                }

                // fill placeholder
                patch_placeholder(placeholder_true);

                // false
                CHECK(compile(cd, ast->right->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile \"false\" side in if statement");

                if (ast->right->right) {
                    patch_placeholder(placeholder_false);
                }

                return 0;
            }

        case NODE_WHILE:
            {
                uint32_t start_index = data->n_program_bytes;

                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile expression in while statement");

                add_instruction(JMP_NOT);
                uint32_t placeholder_false = create_placeholder();

                // body
                CHECK(compile(cd, ast->right->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile while statement body");

                add_instruction(JMP);
                add_number(start_index);

                // fill placeholder
                patch_placeholder(placeholder_false);

                return 0;
            }

        case NODE_CONSTANT:
            {
                const char* var_name = ast->token.value;
                add_variable(var_name, current_scope, *current_stack_index, true, cd);
                increment_index();

                if (ast->left == NULL) {
                    add_instruction(PUSH_FALSE);
                } else {
                    CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile initialization value");
                }

                return 0;
            }
        case NODE_DECLARATION:
            {
                const char* var_name = ast->token.value;
                add_variable(var_name, current_scope, *current_stack_index, false, cd);
                increment_index();

                if (ast->left == NULL) {
                    add_instruction(PUSH_FALSE);
                } else {
                    CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile initialization value");
                }

                return 0;
            }
        case NODE_ASSIGN:
            {
                int32_t placeholder;
                if (ast->left->type == NODE_INDEX) {
                    add_instruction(PUSH_BASE);
                    add_instruction(PUSH_ADDR);
                    placeholder = create_placeholder();
                }

                // first compile value to assign
                CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile assignment value");

                // then compile the lvalue
                CHECK(compile_lvalue(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile lvalue");

                if (ast->left->type == NODE_INDEX) {
                    patch_placeholder(placeholder);
                }
                return 0;
            }
        case NODE_STATEMENT:
            {
                CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile next statement");
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile current statement");

                return 0;
            }

        case NODE_BLOCK:
            {
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope + 1, ctx), "failed to compile current statement");

                uint32_t n_cleaned = pop_variables(current_scope, cd);
                for (uint32_t i = 0; i < n_cleaned; ++i) {
                    add_instruction(POP);
                }

                return 0;
            }
        case NODE_NOT:
            {
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile not statement");
                add_instruction(NOT);

                return 0;
            }
        case NODE_EXPORT:
            {
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile export");
                return 0;
            }

        case NODE_CLASS:
            {
                const char* class_name = ast->token.value;
                struct object cls = {
                    .type = OBJ_CLASS,
                    .obj_value = &(struct object_class) {
                        .type = USER,
                        .constructor = -1,
                        .index = data->n_program_bytes,
                    }
                };

                // compile members
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, cls.obj_value), "failed to compile class members");

                // compile methods
                CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, cls.obj_value), "failed to compile class methods");

                int32_t out_address;
                add_constant(data, &cls, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_variable(class_name, current_scope, *current_stack_index, false, cd);
                increment_index();

                return 0;
            }

        case NODE_MEMBER:
            {
                compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx);

                struct object_class* current_class = ctx;
                current_class->members[current_class->n_members++] = ast->token.value;

                return 0;
            }

        case NODE_METHODS:
            {
                CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile next method");
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile next method");

                return 0;
            }

        case NODE_METHOD:
            {
                add_instruction(JMP);
                uint32_t placeholder = create_placeholder();

                struct object_class* current_class = ctx;

                int32_t method_address = data->n_program_bytes;

                uint32_t new_stack_index = 0;

                struct node* parameter = ast->left;
                uint32_t n_parameters = 0;
                while (parameter) {
                    add_variable(parameter->token.value, current_scope + 1, new_stack_index++, false, cd);
                    parameter = parameter->right;
                    ++n_parameters;
                }

                add_variable("self", current_scope + 1, new_stack_index++, true, cd);
                ++n_parameters;

                CHECK(compile(cd, ast->right, data, &new_stack_index, function_scope + 1, current_scope, ctx), "failed to compile function body");

                current_class->methods[current_class->n_methods++] = (struct named_function){
                    .name = ast->token.value,
                    .function = {
                        .type = USER,
                        .index = method_address,
                        .n_parameters = n_parameters
                    }
                };

                if (strcmp(ast->token.value, "constructor") == 0) {
                    current_class->constructor = current_class->n_methods - 1;

                    add_instruction(DUP_LOC);
                    add_number(n_parameters - 1);
                } else {
                    add_instruction(PUSH_FALSE);
                }

                add_instruction(RET);
                patch_placeholder(placeholder);

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

                const char* fun_name = ast->token.value;
                struct object o = {
                    .type = OBJ_FUNCTION,
                    .obj_value = &(struct object_function) {
                        .type = USER,
                        .n_parameters = n_parameters,
                        .index = data->n_program_bytes + sizeof(int32_t) + sizeof(int32_t) + 2
                   }
                };

                int32_t out_address;
                add_constant(data, &o, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_variable(fun_name, current_scope, *current_stack_index, false, cd);
                increment_index();

                add_instruction(JMP);
                uint32_t placeholder = create_placeholder();

                uint32_t new_stack_index = 0;
                parameter = ast->left;
                while (parameter) {
                    --n_parameters;
                    add_variable(parameter->token.value, current_scope + 1, new_stack_index++, false, cd);

                    parameter = parameter->right;
                }

                add_variable("self", current_scope + 1, new_stack_index++, true, cd);
                ++n_parameters;

                CHECK(compile(cd, ast->right, data, &new_stack_index, function_scope + 1, current_scope, ctx), "failed to compile function body");

                // TODO: double ret in case of return
                add_instruction(RET);
                patch_placeholder(placeholder);

                return 0;
            }

        case NODE_CALL:
            {

                add_instruction(PUSH_BASE);
                add_instruction(PUSH_ADDR);
                int32_t placeholder = create_placeholder();

                uint32_t n_arguments = 0;
                struct node* argument = ast->right;
                while (argument) {
                    CHECK(compile(cd, argument->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile function argument");
                    argument = argument->right;
                    ++n_arguments;
                }

                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile lvalue to call");

                add_instruction(CALL);
                add_number(n_arguments);

                patch_placeholder(placeholder);

                return 0;
            }
        case NODE_MEMBER_ACCESS:
            {
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile left member of member access");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = ast->token.value}, &out_address);

                add_instruction(PUSH);
                add_number(out_address);

                add_instruction(GET_FIELD);

                return 0;
            }
        case NODE_INDEX:
            {
                add_instruction(PUSH_BASE);
                add_instruction(PUSH_ADDR);

                int32_t placeholder = create_placeholder();

                CHECK(compile(cd, ast->right, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile left member of member index expression");

                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile left member of member index");

                int32_t out_address;
                add_constant(data, &(struct object){.type = OBJ_STRING, .str_value = "__get"}, &out_address);
                add_instruction(PUSH)
                add_number(out_address);

                add_instruction(GET_FIELD);

                add_instruction(CALL);
                add_number(1);

                patch_placeholder(placeholder);

                return 0;
            }
        case NODE_RETURN:
            {
                if (!ast->left) {
                    add_instruction(PUSH_FALSE);
                } else {
                    CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile return value");
                }

                add_instruction(RET);

                return 0;
            }
        case NODE_EXP_STATEMENT:
            {
                CHECK(compile(cd, ast->left, data, current_stack_index, function_scope, current_scope, ctx), "failed to compile expression statement");
                add_instruction(POP);

                return 0;
            }

        default:
            return 1;
    }

    return 0;
}


int compile_program(struct sylk* s, struct node* ast, uint8_t* bytecodes, size_t* out_n_bytecodes, uint32_t* out_start_address) {
    struct compiler_data cd = {
        .functions = s->builtin_functions,
        .n_functions = s->n_builtin_functions,
        .classes = s->builtin_classes,
        .n_classes = s->n_builtin_classes
    };

    struct binary_data d = {};
    uint32_t current_stack_index = 0;

    if (compile(&cd, ast, &d, &current_stack_index, 0, -1, NULL) != 0) {
        ERROR("failed to evaluate");
        return 1;
    }

    size_t n_bytecodes = 0;
    memcpy(bytecodes, d.constants_bytes, d.n_constants_bytes);
    n_bytecodes += d.n_constants_bytes;

    uint32_t start_address = n_bytecodes;

    // program start address
    memcpy(bytecodes + n_bytecodes, d.program_bytes, d.n_program_bytes);
    n_bytecodes += d.n_program_bytes;

    *out_n_bytecodes = n_bytecodes;
    *out_start_address = start_address;
    return 0;
}
