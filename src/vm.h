#ifndef VM_H_
#define VM_H_

#include <stdbool.h>
#include <stddef.h>
#include "compiler.h"
#include "gc.h"
#include "objects.h"

#define push(o) \
    vm->stack[vm->stack_size++] = o

#define pop() \
    (vm->stack[--vm->stack_size])

#define peek(n) \
    (vm->stack[vm->stack_size - 1 - n])

struct vm {
    struct function* builtin_functions;
    struct class_* builtin_classes;

    uint32_t globals[2048];
    struct object stack[2048];

    uint8_t* bytes;
    uint32_t n_bytes;

    uint32_t start_address;

    uint32_t stack_size;
    uint32_t stack_base;
    uint32_t program_counter;

    struct object registers[10];

    struct gc gc;

    bool halt;
};

int execute(struct vm* vm);

#endif
