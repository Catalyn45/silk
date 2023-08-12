#ifndef COMPILER_H_
#define COMPILER_H_

#include <stdbool.h>
#include <stdint.h>

#include "ast.h"

struct vm;
typedef struct object (*builtin_fun)(struct vm* vm);
typedef struct object (*builtin_method)(struct object self, struct vm* vm);

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
    builtin_fun fun;
};

struct method {
    const char* name;
    builtin_method method;
};

struct class_ {
    const char* name;
    uint32_t index;

    builtin_method constructor;

    const char* members[20];
    uint32_t n_members;

    struct method methods[20];
    uint32_t n_methods;
};

struct evaluator {
    struct function functions[1024];
    uint32_t n_functions;

    struct class_ classes[256];
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

int evaluate(struct evaluator* e, struct node* ast, struct binary_data* data, uint32_t* current_stack_index, uint32_t function_scope, int32_t current_scope, void* ctx);

#endif
