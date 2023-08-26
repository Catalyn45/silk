#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stddef.h>

struct sylk_named_function;
int add_builtin_functions(struct sylk_named_function* functions, size_t* n_functions);

#endif
