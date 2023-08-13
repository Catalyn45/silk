#ifndef OPERATIONS_H_
#define OPERATIONS_H_
#include <stdint.h>

struct vm;
struct object;

typedef int (*operation_fun)(struct vm* vm, struct object* op1, struct object* op2, struct object* result);
typedef int (*call_fun)(struct vm* vm, struct object* callable, int32_t n_args);

extern operation_fun addition_table[];
extern operation_fun equality_table[];
extern call_fun callable_table[];

#endif
