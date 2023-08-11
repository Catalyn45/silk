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

static void push_number(struct vm* vm, int32_t number) {
    vm->stack[vm->stack_size++] = (struct object){.type = OBJ_NUMBER, .int_value = number};
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
    return obj.int_value;
}

static int pop_number_check(struct vm* vm, int32_t* out_number) {
    struct object obj = pop();
    if (obj.type != OBJ_NUMBER) {
        ERROR("number required, got %d", obj.type);
        return 1;
    }

    *out_number = obj.int_value;
    return 0;
}


static const char* pop_string(struct vm* vm) {
    struct object obj = pop();
    return obj.str_value;
}


// static int pop_string_check(struct vm* vm, const char** out_string) {
//     struct object obj = pop();
//     if (obj.type != OBJ_STRING) {
//         ERROR("string required, got %d", obj.type);
//         return 1;
//     }
//
//     *out_string = obj.str_value;
//     return 0;
// }

bool pop_bool(struct vm* vm) {
    struct object obj = pop();
    return obj.bool_value;
}

bool pop_bool_check(struct vm* vm, bool* out_bool) {
    struct object obj = pop();
    if (obj.type != OBJ_BOOL) {
        ERROR("bool required, got %d", obj.type);
        return 1;
    }

    *out_bool = obj.bool_value;
    return 0;
}

static void call(struct vm* vm, int32_t index) {
    uint32_t old_base = vm->stack_base;
    vm->stack_base = vm->stack_size;

    push_number(vm, vm->program_counter + 1);
    push_number(vm, old_base);

    vm->program_counter = vm->start_address + index - 1;
}

static void ret(struct vm* vm) {
    vm->stack_size = vm->stack_base + 2;
    vm->stack_base = pop_number(vm);
    int32_t index = pop_number(vm);

    vm->program_counter = index - 1;
}

static int add_strings(struct vm* vm, struct object* value1, struct object* value2) {
    char* new_string = malloc(500);
    uint32_t n_new_string = 0;

    if (value1->type == OBJ_NUMBER) {
        n_new_string += sprintf(new_string, "%d", value1->int_value);
    } else if (value1->type == OBJ_STRING) {
        strcpy(new_string, value1->str_value);
        n_new_string += strlen(value1->str_value);
    } else {
        return 1;
    }

    if (value2->type == OBJ_NUMBER) {
        n_new_string += sprintf(new_string + n_new_string, "%d", value2->int_value);
    } else if (value2->type == OBJ_STRING) {
        strcpy(new_string + n_new_string, value2->str_value);
        n_new_string += strlen(value2->str_value);
    } else {
        return 1;
    }

    push_string(vm, new_string);
    return 0;
}


#define read_value(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1)))

#define read_value_increment(value_type) \
    (*((value_type*)(vm->bytes + vm->program_counter + 1))); vm->program_counter += sizeof(value_type)


int execute(struct vm* vm) {
    vm->program_counter = vm->start_address;
    vm->gc.treshold = 2;

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
                --vm->stack_size;
                break;

            case ADD:
                {
                    struct object value1 = pop();
                    struct object value2 = pop();

                    if (value1.type == OBJ_STRING || value2.type == OBJ_STRING) {
                        CHECK(add_strings(vm, &value1, &value2), "failed to add objects");
                        break;
                    }

                    if (value1.type == OBJ_NUMBER && value2.type == OBJ_NUMBER) {
                        int32_t number1 = value1.int_value;
                        int32_t number2 = value2.int_value;

                        push_number(vm, number1 + number2);
                        break;
                    }

                    ERROR("add operation for object types: %d and %d not defined", value1.type, value2.type);
                    return 1;

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

                    if (exp1.type == OBJ_NUMBER && exp2.type == OBJ_NUMBER) {
                        push_bool(vm, exp1.int_value == exp2.int_value);
                        break;
                    }

                    if (exp1.type == OBJ_BOOL && exp2.type == OBJ_BOOL) {
                        push_bool(vm, exp1.bool_value == exp2.bool_value);
                        break;
                    }

                    push_bool(vm, false);
                }
                break;

            case NEQ:
                {
                    struct object exp1 = pop();
                    struct object exp2 = pop();

                    if (exp1.type == OBJ_NUMBER && exp2.type == OBJ_NUMBER) {
                        push_bool(vm, exp1.int_value != exp2.int_value);
                        break;
                    }

                    if (exp1.type == OBJ_BOOL && exp2.type == OBJ_BOOL) {
                        push_bool(vm, exp1.bool_value != exp2.bool_value);
                        break;
                    }

                    push_bool(vm, true);
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
            case DUP_REG:
                {
                    int32_t index = read_value_increment(int32_t);
                    push(vm->registers[index]);
                }
                break;
            case CHANGE:
                {
                    int32_t index = pop_number(vm);
                    vm->stack[index] = pop();
                }
                break;
            case CHANGE_LOC:
                {
                    int32_t index = pop_number(vm);
                    vm->stack[vm->stack_base + index] = pop();
                }
                break;
            case CHANGE_REG:
                {
                    int32_t index = read_value_increment(int32_t);
                    vm->registers[index] = pop();
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

            case CALL:
                {
                    struct object o = pop();

                    if (o.type == OBJ_CLASS) {
                        struct object_class* cls = o.class_value;

                        struct object_instance* value = gc_alloc(vm, sizeof(*value));
                        if (value == NULL)
                            return 1;

                        if (cls->type == USER) {
                            *value = (struct object_instance) {
                                .type = USER,
                                .class_index = cls
                            };
                        } else {
                            *value = (struct object_instance) {
                                .type = BUILT_IN,
                                .buintin_index = &vm->builtin_classes[cls->index]
                            };
                        }

                        struct object o = (struct object) {
                            .type = OBJ_INSTANCE,
                            .instance_value = value
                        };

                        if (cls->type == BUILT_IN) {
                            vm->builtin_classes[cls->index].methods[0].method(o, vm);
                        } else {
                            uint32_t i;
                            for (i = 0; i < cls->n_methods; ++i) {
                                if (strcmp(cls->methods[i].name, "constructor") == 0) {
                                    push(o);
                                    call(vm, cls->methods[i].index);
                                    break;
                                }
                            }

                            if (i == cls->n_methods)
                                return 1;
                        }

                        vm->registers[RETURN_REGISTER] = o;
                        break;
                    }

                    if (o.type == OBJ_METHOD) {
                        if (o.method_value->type == USER) {
                            push(o.method_value->context);
                            call(vm, o.method_value->index);
                        } else if (o.method_value->type == BUILT_IN) {
                            struct object* instance = &o.method_value->context;
                            struct class_* cls = instance->instance_value->buintin_index;
                            struct method* method = &cls->methods[o.method_value->index];

                            vm->registers[RETURN_REGISTER] = method->method(*instance, vm);
                        }
                        break;
                    }

                    if (o.type == OBJ_FUNCTION) {
                        if (o.function_value->type == USER) {
                            call(vm, o.function_value->index);
                        } else if (o.function_value->type == BUILT_IN) {
                            vm->registers[RETURN_REGISTER] = vm->builtin_functions[o.function_value->index].fun(vm);
                        }
                    }
                }
                break;

            case GET_FIELD:
                {
                    const char* field_name = pop_string(vm);

                    struct object instance = pop();

                    if (instance.instance_value->type == BUILT_IN) {
                        struct class_* cls = instance.instance_value->buintin_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                push(instance.instance_value->members[i]);
                                break;
                            }
                        }

                        if (i == cls->n_members) {
                            for (i = 0; i < cls->n_methods; ++i) {
                                if (strcmp(cls->methods[i].name, field_name) == 0) {
                                    push_method(vm, &(struct object_method){.type = BUILT_IN, .index = i, .n_parameters = 0, .context = instance});
                                    break;
                                }
                            }
                        }

                        if (i == cls->n_methods)
                            return 1;

                        break;
                    }

                    struct object_class* cls = instance.instance_value->class_index;
                    uint32_t i = 0;
                    for (i = 0; i < cls->n_members; ++i) {
                        if (strcmp(cls->members[i], field_name) == 0) {
                            push(instance.instance_value->members[i]);
                            break;
                        }
                    }

                    if (i == cls->n_members) {
                        for (i = 0; i < cls->n_methods; ++i) {
                            if (strcmp(cls->methods[i].name, field_name) == 0) {
                                push_method(vm, &(struct object_method){.type = USER, .index = cls->methods[i].index, .n_parameters = cls->methods[i].n_parameters, .context = instance});
                                break;
                            }
                        }
                    }

                    if (i == cls->n_methods)
                        return 1;
                }
                break;

            case SET_FIELD:
                {
                    const char* field_name = pop_string(vm);

                    struct object instance = pop();

                    if (instance.instance_value->type == BUILT_IN) {
                        struct class_* cls = instance.instance_value->buintin_index;

                        uint32_t i;
                        for (i = 0; i < cls->n_members; ++i) {
                            if (strcmp(cls->members[i], field_name) == 0) {
                                instance.instance_value->members[i] = pop();
                                break;
                            }
                        }

                        if (i == cls->n_members)
                            return 1;

                        break;
                    }

                    struct object_class* cls = instance.instance_value->class_index;
                    uint32_t i;
                    for (i = 0; i < cls->n_members; ++i) {
                        if (strcmp(cls->members[i], field_name) == 0) {
                            instance.instance_value->members[i] = pop();
                            break;
                        }
                    }

                    if (i == cls->n_members)
                        return 1;
                }
                break;
            case RET:
                {
                    ret(vm);
                }
                break;
            case RET_INS:
                {
                    vm->registers[RETURN_REGISTER] = vm->stack[vm->stack_base - 1];
                    ret(vm);
                    --vm->stack_size;
                }
                break;
        }

        ++vm->program_counter;
    }

    return 0;
}

