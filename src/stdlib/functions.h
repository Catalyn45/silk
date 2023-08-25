#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stddef.h>

struct named_function;
int add_builtin_functions(struct named_function* functions, size_t* n_functions);

#endif
