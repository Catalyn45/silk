#ifndef VM_H_
#define VM_H_

#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

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
    CALL,
    RET
};

struct var {
    const char* name;
    void* value;
    uint32_t scope;
    int32_t stack_index;
};

struct function {
    const char* name;

    uint32_t n_parameters;
    uint32_t index;
};

struct evaluator {
    struct var constants[1024];
    uint32_t n_constants;

    struct function functions[1024];
    uint32_t n_functions;

    struct var locals[1024];
    uint32_t n_locals;

};

struct binary_data {
    uint8_t constants_bytes[2048];
    uint32_t n_constants_bytes;

    uint8_t function_bytes[2048];
    uint32_t n_function_bytes;

    uint8_t program_bytes[2048];
    uint32_t n_program_bytes;
};

int evaluate(struct node* ast, uint8_t* bytes, uint32_t* n_bytes, struct binary_data* data, uint32_t* current_stack_index, struct evaluator* e);

struct vm {
    uint32_t globals[2048];
    uint32_t stack[2048];

    uint8_t* bytes;
    uint32_t n_bytes;

    bool halt;
};

int execute(struct vm* vm);

enum predefined_indexes {
    STACK_SIZE_INDEX = 0,
    STACK_BASE_INDEX = 1,
    RETURN_INDEX = 2,
    PROGRAM_COUNTER = 3
};

#endif
