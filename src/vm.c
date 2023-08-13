#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include "vm.h"
#include "parser.h"
#include "stdlib.h"
#include "utils.h"
#include "instructions.h"
#include "gc.h"
#include "objects.h"
#include "operations.h"

static void push_number(struct vm* vm, int32_t number) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_NUMBER, .num_value = number};
}

static void push_string(struct vm* vm, char* string) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_STRING, .str_value = string};
}

static void push_bool(struct vm* vm, bool value) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_BOOL, .bool_value = value};
}

static void push_func(struct vm* vm, struct object_function* func) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_FUNCTION, .function_value = func};
}

static void push_method(struct vm* vm, struct object_method* method) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_METHOD, .method_value = method};
}

static void push_class(struct vm* vm, struct object_class* cls) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_CLASS, .class_value = cls};
}

static void push_constant(struct vm* vm, int32_t address) {
    int type = *((int32_t*)&vm->bytes[address]);
    address += sizeof(int32_t);

    void* value;
    if (type == OBJ_NUMBER) {
        int32_t number = *((int32_t*)&vm->bytes[address]);
        push_number(vm, number);
        return;
    }

    if (type == OBJ_STRING) {
        value = &vm->bytes[address];
        push_string(vm, value);
        return;
    }

    if (type == OBJ_FUNCTION) {
        struct object_function* func = (struct object_function*)&vm->bytes[address];
        push_func(vm, func);
        return;
    }

    if (type == OBJ_CLASS) {
        struct object_class* cls = (struct object_class*)&vm->bytes[address];
        push_class(vm, cls);
        return;
    }
}

static int32_t pop_number(struct vm* vm) {
    struct object obj = pop();
    return obj.num_value;
}

static int pop_number_check(struct vm* vm, int32_t* out_number) {
    struct object obj = pop();
    EXPECT_OBJECT(obj.type, OBJ_NUMBER);

    *out_number = obj.num_value;
    return 0;
}

static const char* pop_string(struct vm* vm) {
    struct object obj = pop();
    return obj.str_value;
}

// static int pop_string_check(struct vm* vm, const char** out_string) {
//     struct object obj = pop();
//     EXPECT_OBJECT(obj.type, OBJ_STRING);
//
//     *out_string = obj.str_value;
//     return 0;
// }

bool pop_bool(struct vm* vm) {
    struct object obj = pop();
    return obj.bool_value;
}

int pop_bool_check(struct vm* vm, bool* out_bool) {
    struct object obj = pop();
    EXPECT_OBJECT(obj.type, OBJ_BOOL);

    *out_bool = obj.bool_value;
    return 0;
}

static void ret(struct vm* vm) {
    struct object return_val = pop();

    vm->stack_size = vm->stack_base;
    int32_t index = pop_number(vm);
    vm->stack_base = pop_number(vm);

    push(return_val);
    vm->program_counter = index - 1;
}

#define read_value(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1)))

#define read_value_increment(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1))); vm->program_counter += sizeof(value_type)

int execute(struct vm* vm) {
    vm->program_counter = vm->start_address;

    // run gc at this number of allocated objects
    vm->gc.treshold = 20;

    while (!vm->halt && vm->program_counter < vm->n_bytes) {
        switch (vm->bytes[vm->program_counter]) {
            case PUSH:
                {
                    int32_t constant = read_value_increment(int32_t);
                    push_constant(vm, constant);
                }
                break;

            case PUSH_NUM:
                {
                    int32_t number = read_value_increment(int32_t);
                    push_number(vm, number);
                }
                break;

            case PUSH_TRUE:
                {
                    push_bool(vm, true);
                }
                break;

            case PUSH_FALSE:
                {
                    push_bool(vm, false);
                }
                break;

            case POP:
                (void)pop();
                break;

            case ADD:
                {
                    struct object value1 = pop();
                    struct object value2 = pop();

                    operation_fun operation = addition_table[value1.type];
                    CHECK_NULL(operation, "can't add objects of type: %s and %s", rev_objects[value1.type], rev_objects[value2.type]);

                    struct object result;
                    CHECK(operation(vm, &value1, &value2, &result), "failed to add objects");

                    push(result);
                }
                break;

            case MIN:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for minus operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for minus operation is not a number");

                    push_number(vm, number1 - number2);
                }
                break;

            case MUL:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for multiply operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for multiply operation is not a number");

                    push_number(vm, number1 * number2);
                }
                break;

            case DIV:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for division operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for division operation is not a number");

                    push_number(vm, number1 / number2);
                }
                break;

            case NOT:
                {
                    bool exp;
                    CHECK(pop_bool_check(vm, &exp), "operand for not operation is not bool");

                    push_bool(vm, !exp);
                }
                break;

            case DEQ:
                {
                    struct object exp1 = pop();
                    struct object exp2 = pop();

                    operation_fun operation = equality_table[exp1.type];
                    CHECK_NULL(operation, "equality not possible for operands of type: %s and %s", rev_objects[exp1.type], rev_objects[exp2.type]);

                    struct object result;
                    CHECK(operation(vm, &exp1, &exp2, &result), "failed to add objects");

                    push(result);
                }
                break;

            case NEQ:
                {
                    struct object exp1 = pop();
                    struct object exp2 = pop();

                    operation_fun operation = equality_table[exp1.type];
                    CHECK_NULL(operation, "difference not possible for operands of type: %s and %s", rev_objects[exp1.type], rev_objects[exp2.type]);

                    struct object result;
                    CHECK(operation(vm, &exp1, &exp2, &result), "failed to add objects");

                    result.bool_value = !result.bool_value;
                    push(result);
                }
                break;

            case GRE:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for greater operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for greater operation is not a number");

                    push_bool(vm, number1 > number2);
                }
                break;

            case GRQ:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for greater or equal operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for greater or equal operation is not a number");

                    push_bool(vm, number1 >= number2);
                }
                break;

            case LES:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for less operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for less operation is not a number");

                    push_bool(vm, number1 < number2);
                }
                break;

            case LEQ:
                {
                    int32_t number1;
                    CHECK(pop_number_check(vm, &number1), "operand 1 for less or equal operation is not a number");

                    int32_t number2;
                    CHECK(pop_number_check(vm, &number2), "operand 2 for less or equal operation is not a number");

                    push_bool(vm, number1 <= number2);
                }
                break;

            case AND:
                {
                    bool exp1;
                    CHECK(pop_bool_check(vm, &exp1), "operand 1 for and operation is not a number");

                    bool exp2;
                    CHECK(pop_bool_check(vm, &exp2), "operand 2 for and operation is not a number");

                    push_bool(vm, exp1 && exp2);
                }
                break;

            case OR:
                {
                    bool exp1;
                    CHECK(pop_bool_check(vm, &exp1), "operand 1 for or operation is not a number");

                    bool exp2;
                    CHECK(pop_bool_check(vm, &exp2), "operand 2 for or operation is not a number");

                    push_bool(vm, exp1 || exp2);
                }
                break;

            case DUP:
                {
                    int32_t index = read_value_increment(int32_t);
                    push(vm->stack[index]);
                }
                break;

            case DUP_LOC:
                {
                    int32_t index = read_value_increment(int32_t);
                    push(vm->stack[vm->stack_base + index]);
                }
                break;
            case CHANGE:
                {
                    int32_t index = read_value_increment(int32_t);
                    vm->stack[index] = pop();
                }
                break;
            case CHANGE_LOC:
                {
                    int32_t index = read_value_increment(int32_t);
                    vm->stack[vm->stack_base + index] = pop();
                }
                break;
            case JMP_NOT:
                {
                    int32_t index = read_value_increment(int32_t);

                    bool condition;
                    CHECK(pop_bool_check(vm, &condition), "operand for jump operation is not bool");

                    if (!condition) {
                        vm->program_counter = vm->start_address + index - 1;
                    }
                }
                break;

            case JMP:
                {
                    int32_t index = read_value(int32_t);
                    vm->program_counter = vm->start_address + index - 1;
                }
                break;

            case PUSH_BASE:
                {
                    uint32_t old_base = vm->stack_base;
                    push_number(vm, old_base);
                }
                break;

            case PUSH_ADDR:
                {
                    int32_t addr = read_value_increment(int32_t);
                    push_number(vm, vm->start_address + addr);
                }
                break;

            case CALL:
                {
                    struct object o = pop();
                    int32_t n_args = read_value_increment(int32_t);

                    call_fun call_cb = callable_table[o.type];
                    CHECK_NULL(call_cb, "object of type %s can't be called", rev_tokens[o.type]);

                    CHECK(call_cb(vm, &o, n_args), "failed to call object of type: %d", o.type);
                }

                break;

            case GET_FIELD:
                {
                    const char* field_name = pop_string(vm);
                    struct object instance = pop();

                    EXPECT_OBJECT(instance.type, OBJ_INSTANCE);

                    struct object_instance* instance_value = instance.instance_value;

                    if (instance_value->type == BUILT_IN) {
                        struct class_* cls = instance_value->buintin_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                push(instance_value->members[i]);
                                break;
                            }
                        }

                        if (i < cls->n_members) {
                            break;
                        }

                        for (i = 0; i < cls->n_methods; ++i) {
                            if (strcmp(cls->methods[i].name, field_name) == 0) {
                                push_method(vm, &(struct object_method){
                                    .type = BUILT_IN,
                                    .index = i,
                                    .n_parameters = 0,
                                    .context = instance
                                });
                                break;
                            }
                        }

                        if (i < cls->n_methods) {
                            break;
                        }

                        ERROR("attribute: %s does not exist in class: %s", field_name, cls->name);
                        return 1;
                    }

                    if (instance_value->type == USER) {
                        struct object_class* cls = instance_value->class_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                push(instance_value->members[i]);
                                break;
                            }
                        }

                        if (i < cls->n_members) {
                            break;
                        }

                        for (i = 0; i < cls->n_methods; ++i) {
                            if (strcmp(cls->methods[i].name, field_name) == 0) {
                                struct pair* method = &cls->methods[i];

                                push_method(vm, &(struct object_method){
                                    .type = USER,
                                    .index = method->index,
                                    .n_parameters = method->n_parameters,
                                    .context = instance
                                });
                                break;
                            }
                        }

                        if (i < cls->n_methods) {
                            break;
                        }

                        ERROR("attribute: %s does not exist in class: %s", field_name, cls->name);
                        return 1;
                    }
                }
                break;

            case SET_FIELD:
                {
                    const char* field_name = pop_string(vm);
                    struct object instance = pop();

                    EXPECT_OBJECT(instance.type, OBJ_INSTANCE);

                    struct object_instance* instance_value = instance.instance_value;

                    if (instance_value->type == BUILT_IN) {
                        struct class_* cls = instance_value->buintin_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                instance_value->members[i] = pop();
                                break;
                            }
                        }

                        if (i == cls->n_members) {
                            ERROR("property: %s does not exist in class: %s", field_name, cls->name);
                            return 1;
                        }

                        break;
                    }

                    if (instance_value->type == USER) {
                        struct object_class* cls = instance_value->class_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                instance_value->members[i] = pop();
                                break;
                            }
                        }

                        if (i == cls->n_members) {
                            ERROR("property: %s does not exist in class: %s", field_name, cls->name);
                            return 1;
                        }
                    }
                }
                break;
            case RET:
                {
                    ret(vm);
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

