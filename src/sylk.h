#ifndef SILK_H_
#define SILK_H_

#include <stddef.h>
#include <stdbool.h>

struct sylk;

struct sylk_config {
    bool print_ast;
    bool print_bytecode;
    bool halt_program;
};

struct sylk* sylk_new(const struct sylk_config* config);
void sylk_free(struct sylk* s);

int sylk_load_functions(struct sylk* s);
int sylk_load_prelude(struct sylk* s);
int sylk_run_file(struct sylk* s, const char* file_name);
int sylk_run_string(struct sylk* s, const char* program, size_t program_size);

#endif
