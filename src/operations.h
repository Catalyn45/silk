#ifndef OPERATIONS_H_
#define OPERATIONS_H_
#include <stdint.h>

struct vm;
struct object;

typedef int (*operation_fun)(struct vm* vm, struct object* op1, struct object* op2, struct object* result);
typedef int (*call_fun)(struct vm* vm, struct object* callable, int32_t n_args);
typedef int (*field_fun)(struct vm* vm, struct object* instance, const char* field_name);

extern operation_fun addition_table[];
extern operation_fun equality_table[];
extern call_fun callable_table[];
extern field_fun get_table[];
extern field_fun set_table[];

#endif
