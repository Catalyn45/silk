#ifndef VM_H_
#define VM_H_

#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

struct vm {
    uint32_t stack[1024];
    uint32_t stack_size;

    uint8_t* bytes;
    uint32_t n_bytes;

    uint32_t program_counter;

    bool halt;
};

int evaluate(struct node* ast, uint8_t* bytes, uint32_t* n_bytes, uint32_t* current_stack_size);

int execute(struct vm* vm);

#endif
