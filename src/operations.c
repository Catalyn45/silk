#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "operations.h"
#include "objects.h"
#include "vm.h"

static int add_strings(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    EXPECT_OBJECT(op2->type, OBJ_STRING);

    char* new_string = gc_alloc(vm, OBJ_STRING, 500);
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

    struct object_class* cls = callable->obj_value;

    struct object_instance* value = gc_alloc(vm, OBJ_INSTANCE, sizeof(*value));
    CHECK_MEM(value);

    *value = (struct object_instance) {
        .cls = cls
    };

    struct object o = (struct object) {
        .type = OBJ_INSTANCE,
        .obj_value = value
    };

    struct object_function* constructor = NULL;
    if (cls->constructor >= 0) {
        constructor = &cls->methods[cls->constructor].function;
    }

    if (cls->type == BUILT_IN) {
        if (constructor) {
            constructor->function(&o, vm);
        }

        pop_args(n_args);
        push(o);
        return 0;
    }

    if (cls->type == USER) {
        if(constructor) {
            push(o);
            call(vm, constructor->index, n_args + 1);
            return 0;
        }

        pop_args(n_args);
        push(o);
        return 0;
    }

    ERROR("class type: %d unknown", cls->type);
    return 1;
}

static int call_function(struct vm* vm, struct object* callable, int32_t n_args) {
    struct object_function* function_value = callable->obj_value;

    if (function_value->type == USER) {
        push(function_value->context);
        call(vm, function_value->index, n_args + 1);
        return 0;
    }

    if (function_value->type == BUILT_IN) {
        struct object result = function_value->function(&function_value->context, vm);

        pop_args(n_args);

        push(result);
        return 0;
    }

    ERROR("function of type: %d unknown", function_value->type);
    return 1;
}

call_fun callable_table[OBJ_COUNT] = {
    [OBJ_CLASS] = call_class,
    [OBJ_FUNCTION] = call_function,
};

static int get_instance(struct vm* vm, struct object* instance, const char* field_name) {
    struct object_instance* instance_value = instance->obj_value;
    struct object_class* cls = instance_value->cls;

    uint32_t i;
    for (i = 0; i < cls->n_members; ++i) {
        if (cls->members[i] && strcmp(cls->members[i], field_name) == 0) {
            push(instance_value->members[i]);
            return 0;
        }
    }

    for (i = 0; i < cls->n_methods; ++i) {
        if (strcmp(cls->methods[i].name, field_name) == 0) {
            struct object_function method = cls->methods[i].function;

            struct object_function* fun = gc_alloc(vm, OBJ_FUNCTION, sizeof(*fun));
            CHECK_MEM(fun);

            *fun = method;
            fun->context = *instance;

            struct object to_push = {
                .type = OBJ_FUNCTION,
                .obj_value = fun
            };

            push(to_push);
            return 0;
        }
    }

    ERROR("attribute: %s does not exist in class", field_name);
    return 1;
}

field_fun get_table[OBJ_COUNT] = {
    [OBJ_INSTANCE] = get_instance
};

static int set_instance(struct vm* vm, struct object* instance, const char* field_name) {
    struct object_instance* instance_value = instance->obj_value;
    struct object_class* cls = instance_value->cls;

    uint32_t i;
    for (i = 0; i < cls->n_members; ++i) {
        if (strcmp(cls->members[i], field_name) == 0) {
            instance_value->members[i] = pop();
            return 0;
        }
    }

    ERROR("property: %s does not exist in class", field_name);
    return 1;
}

field_fun set_table[OBJ_COUNT] = {
    [OBJ_INSTANCE] = set_instance
};
