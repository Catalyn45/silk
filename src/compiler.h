#ifndef COMPILER_H_
#define COMPILER_H_

#include <stdbool.h>
#include <stdint.h>

#include "ast.h"
#include "objects.h"

struct vm;

struct var {
    const char* name;
    void* value;
    uint32_t scope;
    int32_t stack_index;
};

struct evaluator {
    struct named_function functions[1024];
    uint32_t n_functions;

    struct named_class classes[256];
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

int evaluate(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope, void* ctx);

#endif
