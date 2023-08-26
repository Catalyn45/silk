#ifndef SYLK_H_
#define SYLK_H_

#include <stddef.h>
#include <stdbool.h>
#include "objects.h"

struct sylk_config {
    bool print_ast;
    bool print_bytecode;
    bool halt_program;
};

struct sylk_function {
    const char* name;
    builtin_fun function;
    size_t n_parameters;
};

struct sylk_class {
    const char* name;

    const char** members;
    struct sylk_function* methods;
};

struct sylk;
struct sylk* sylk_new(const struct sylk_config* config, void* ctx);
void sylk_free(struct sylk* s);

int sylk_load_prelude(struct sylk* s);
int sylk_load_functions(struct sylk* s, struct sylk_function* functions);
int sylk_load_classes(struct sylk* s, struct sylk_class* classes);

int sylk_run_string(struct sylk* s, const char* program, size_t program_size);
int sylk_run_file(struct sylk* s, const char* file_name);

struct sylk_vm;
int sylk_push(struct sylk_vm* vm, const struct sylk_object* o);
int sylk_pop(struct sylk_vm* vm, struct sylk_object* o);
int sylk_peek(struct sylk_vm* vm, size_t n, struct sylk_object* o);

#endif
