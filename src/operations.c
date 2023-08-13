#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "operations.h"
#include "objects.h"
#include "vm.h"

static int add_strings(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    EXPECT_OBJECT(op2->type, OBJ_STRING);

    char* new_string = gc_alloc(vm, 500);
    CHECK_MEM(new_string);

    uint32_t n_new_string = 0;

    strcpy(new_string, op1->str_value);
    n_new_string += strlen(op1->str_value);

    strcpy(new_string + n_new_string, op2->str_value);
    n_new_string += strlen(op2->str_value);

    *result = (struct object){.type = OBJ_STRING, .str_value = new_string};
    return 0;
}

static int add_numbers(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    (void)vm;
    EXPECT_OBJECT(op2->type, OBJ_NUMBER);

    *result = (struct object){.type = OBJ_NUMBER, .num_value = op1->num_value + op2->num_value};
    return 0;
}

operation_fun addition_table[OBJ_COUNT] = {
    [OBJ_STRING] = add_strings,
    [OBJ_NUMBER] = add_numbers
};

static int eq_numbers(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    (void)vm;
    EXPECT_OBJECT(op2->type, OBJ_NUMBER);

    *result = (struct object){.type = OBJ_BOOL, .bool_value = op1->num_value == op2->num_value};
    return 0;
}

static int eq_bools(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    (void)vm;
    EXPECT_OBJECT(op2->type, OBJ_BOOL);

    *result = (struct object){.type = OBJ_BOOL, .bool_value = op1->bool_value == op2->bool_value};
    return 0;
}

static int eq_strings(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    (void)vm;
    EXPECT_OBJECT(op2->type, OBJ_STRING);

    bool eq_res = strcmp(op1->str_value, op2->str_value) == 0;

    *result = (struct object){.type = OBJ_BOOL, .bool_value = eq_res};
    return 0;
}

operation_fun equality_table[OBJ_COUNT] = {
    [OBJ_NUMBER] = eq_numbers,
    [OBJ_BOOL] = eq_bools,
    [OBJ_STRING] = eq_strings
};

#define pop_args(n_args) \
{ \
    for (int32_t i = 0; i < n_args; ++i) { \
        (void)pop(); \
    } \
    (void)pop(); \
    (void)pop(); \
}

static void call(struct vm* vm, int32_t index, uint32_t n_args) {
    vm->stack_base = vm->stack_size - n_args;
    vm->program_counter = vm->start_address + index - 1;
}

static int call_class(struct vm* vm, struct object* callable, int32_t n_args) {
    gc_clean(vm);

    struct object_instance* value = gc_alloc(vm, sizeof(*value));
    CHECK_MEM(value);

    struct object o = (struct object) {
        .type = OBJ_INSTANCE,
        .instance_value = value
    };

    struct object_class* cls = callable->class_value;

    if (cls->type == BUILT_IN) {
        *value = (struct object_instance) {
            .type = BUILT_IN,
            .buintin_index = &vm->builtin_classes[cls->index]
        };

        if (vm->builtin_classes[cls->index].constructor) {
            vm->builtin_classes[cls->index].constructor(o, vm);
        }

        pop_args(n_args);
        push(o);
        return 0;
    }

    if (cls->type == USER) {
        *value = (struct object_instance) {
            .type = USER,
            .class_index = cls
        };

        if(cls->constructor >= 0) {
            push(o);
            call(vm, cls->constructor, n_args + 1);
        } else {
            pop_args(n_args);
            push(o);
        }

        return 0;
    }

    ERROR("class type: %d unknown", cls->type);
    return 1;
}

static int call_method(struct vm* vm, struct object* callable, int32_t n_args) {
    if (callable->method_value->type == USER) {
        push(callable->method_value->context);
        call(vm, callable->method_value->index, n_args + 1);

        return 0;
    }

    if (callable->method_value->type == BUILT_IN) {
        struct object* instance = &callable->method_value->context;
        struct class_* cls = instance->instance_value->buintin_index;
        struct method* method = &cls->methods[callable->method_value->index];

        struct object result = method->method(*instance, vm);
        pop_args(n_args);

        push(result);

        return 0;
    }

    ERROR("method of type: %d unknown", callable->method_value->type);
    return 1;
}

static int call_function(struct vm* vm, struct object* callable, int32_t n_args) {
    if (callable->function_value->type == USER) {
        call(vm, callable->function_value->index, n_args);

        return 0;
    }

    if (callable->function_value->type == BUILT_IN) {
        struct object result = vm->builtin_functions[callable->function_value->index].fun(vm);
        pop_args(n_args);
        push(result);

        return 0;
    }

    ERROR("function of type: %d unknown", callable->function_value->type);
    return 1;
}

call_fun callable_table[OBJ_COUNT] = {
    [OBJ_CLASS] = call_class,
    [OBJ_FUNCTION] = call_function,
    [OBJ_METHOD] = call_method
};
