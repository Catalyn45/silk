#ifndef OPERATIONS_H_
#define OPERATIONS_H_
#include <stdint.h>

struct sylk_vm;
struct sylk_object;

typedef int (*operation_fun)(struct sylk_vm* vm, struct sylk_object* op1, struct sylk_object* op2, struct sylk_object* result);
typedef int (*call_fun)(struct sylk_vm* vm, struct sylk_object* callable, int32_t n_args, void* ctx);
typedef int (*field_fun)(struct sylk_vm* vm, struct sylk_object* instance, const char* field_name);

extern operation_fun addition_table[];
extern operation_fun equality_table[];
extern call_fun callable_table[];
extern field_fun get_table[];
extern field_fun set_table[];

#endif
