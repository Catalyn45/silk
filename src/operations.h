#ifndef OPERATIONS_H_
#define OPERATIONS_H_

struct vm;
struct object;
typedef int (*operation_fun)(struct vm* vm, struct object* op1, struct object* op2, struct object* result);

extern operation_fun addition_table[];

#endif
