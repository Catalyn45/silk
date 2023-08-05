#ifndef VM_H_
#define VM_H_

#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

enum instructions {
    PUSH       = 0,
    PUSH_TRUE  = 1,
    PUSH_FALSE = 2,
    POP        = 3,
    ADD        = 4,
    MIN        = 5,
    MUL        = 6,
    DIV        = 7,
    NOT        = 8,
    DEQ        = 9,
    NEQ        = 10,
    GRE        = 11,
    GRQ        = 12,
    LES        = 13,
    LEQ        = 14,
    AND        = 15,
    OR         = 16,
    DUP        = 17,
    DUP_LOC    = 18,
    DUP_REG    = 19,
    CHANGE     = 20,
    CHANGE_REG = 21,
    CHANGE_LOC = 22,
    JMP_NOT    = 23,
    JMP        = 24,
    CALL       = 25,
    RET        = 26,
    PUSH_NUM   = 27
};

#define push(o) \
    vm->stack[vm->stack_size++] = o

#define pop() \
    (&vm->stack[--vm->stack_size])

#define peek(n) \
    (&vm->stack[vm->stack_size - 1 - n])

struct var {
    const char* name;
    void* value;
    uint32_t scope;
    int32_t stack_index;
};

struct vm;
typedef struct object (*builtin_fun)(struct vm* vm);

struct function {
    const char* name;
    uint32_t n_parameters;
    uint32_t index;
    builtin_fun fun;
};

enum object_type {
    OBJ_NUMBER   = 0,
    OBJ_STRING   = 1,
    OBJ_BOOL     = 2,
    OBJ_FUNCTION = 3,
    OBJ_INSTANCE = 4
};


struct object_instance {
    int32_t class_index;
    uint32_t members[256];
};

enum function_type {
    USER     = 0,
    BUILT_IN = 1,
    METHOD   = 3
};

struct object_function {
    int32_t type;
    int32_t index;
    int32_t n_parameters;
};

struct object {
    int32_t type;
    union {
        int32_t int_value;
        const char* str_value;
        bool bool_value;
        struct object_function* function_value;
        struct object_instance* instance_value;
    };
};

struct pair {
    const char* name;
    int32_t index;
};

struct object_class {
    const char* name;

    struct pair members[256];
    uint32_t n_members;

    struct pair methods[256];
    uint32_t n_methods;
};

struct evaluator {
    struct function functions[1024];
    uint32_t n_functions;

    struct object_class classes[256];
    uint32_t n_classes;

    struct var locals[1024];
    uint32_t n_locals;
};

struct binary_data {
    uint8_t constants_bytes[2048];
    uint32_t n_constants_bytes;

    uint8_t classes[2048];
    uint32_t n_classes;

    uint8_t program_bytes[2048];
    uint32_t n_program_bytes;
};

int add_builtin_functions(struct evaluator* e);
int evaluate(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope);

struct vm {
    struct function* builtin_functions;

    uint32_t globals[2048];
    struct object stack[2048];

    uint8_t* bytes;
    uint32_t n_bytes;

    uint32_t stack_size;
    uint32_t stack_base;
    uint32_t program_counter;
    struct object registers[10];

    bool halt;
};

int execute(struct vm* vm);

#endif
