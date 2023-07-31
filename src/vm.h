#ifndef VM_H_
#define VM_H_

#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

enum instructions {
    PUSH,
    PUSH_TRUE,
    PUSH_FALSE,
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
    DUP_REGISTER,
    CHANGE,
    CHANGE_REGISTER,
    JMP_NOT,
    JMP,
    CALL,
    CALL_BUILTIN,
    RET
};

struct var {
    const char* name;
    void* value;
    uint32_t scope;
    int32_t stack_index;
};

enum function_type {
    USER,
    BUILT_IN
};

struct function {
    int type;
    const char* name;

    uint32_t n_parameters;
    uint32_t index;
};


enum object_type {
    OBJ_NUMBER = 0,
    OBJ_STRING = 1,
    OBJ_BOOL = 2,
    OBJ_CLASS = 3
};

struct object {
    int32_t type;
    union {
        int32_t int_value;
        const char* str_value;
        bool bool_value;
        struct object_class* object_class_value;
    };
};

struct object_class {
    int32_t class_index;

    uint32_t n_members;
    struct object members[256];
};

struct evaluator {
    struct function functions[1024];
    uint32_t n_functions;

    struct var locals[1024];
    uint32_t n_locals;
};

struct binary_data {
    uint8_t constants_bytes[2048];
    uint32_t n_constants_bytes;

    uint8_t program_bytes[2048];
    uint32_t n_program_bytes;
};

int initialize_evaluator(struct evaluator* e);
int evaluate(struct node* ast, uint8_t* bytes, uint32_t* n_bytes, struct binary_data* data, uint32_t* current_stack_index, struct evaluator* e);

struct vm {
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
