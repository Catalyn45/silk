#ifndef VM_H_
#define VM_H_

#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

struct var {
    const char* name;
    void* value;
    uint32_t scope;
};

struct evaluator {
    struct var constants[1024];
    uint32_t n_constants;

    struct var globals[1024];
    uint32_t n_globals;

    struct var locals[1024];
    uint32_t n_locals;

};

int evaluate(struct node* ast, uint8_t* bytes, uint32_t* n_bytes, struct evaluator* e);

struct vm {
    uint32_t stack[1024];
    uint32_t stack_size;

    uint8_t* bytes;
    uint32_t n_bytes;

    uint32_t program_counter;

    bool halt;
};

int execute(struct vm* vm);

#endif
