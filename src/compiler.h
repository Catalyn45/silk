#ifndef COMPILER_H_
#define COMPILER_H_

#include <stdbool.h>
#include <stdint.h>

#include "ast.h"
#include "objects.h"

struct vm;
struct sylk;

int compile_program(struct sylk* s, struct node* ast, uint8_t* bytecodes, size_t* out_n_bytecodes, uint32_t* out_start_address);

#endif
