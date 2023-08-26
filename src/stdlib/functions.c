#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "../objects.h"

#include "../vm.h"

static struct sylk_object print_object(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)self;
    (void)ctx;

    struct sylk_object o = pop();

    if (o.type == SYLK_OBJ_NUMBER) {
        printf("%d\n", o.num_value);
    } else if (o.type == SYLK_OBJ_STRING) {
        printf("%s\n", o.str_value);
    } else if (o.type == SYLK_OBJ_BOOL) {
        printf("%s\n", o.bool_value ? "true" : "false");
    } else if (o.type == SYLK_OBJ_CLASS) {
        printf("class\n");
    } else if (o.type == SYLK_OBJ_INSTANCE) {
        printf("instance\n");
    } else if (o.type == SYLK_OBJ_FUNCTION) {
        puts("function");
    } else if (o.type == SYLK_OBJ_NUMBER) {
        puts("user");
    }

    return (struct sylk_object){};
}

static struct sylk_object input_number(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)self;
    (void)ctx;

    struct sylk_object o = pop();

    const char* input_text = o.str_value;
    printf("%s", input_text);

    int32_t number;
    scanf("%d", &number);

    puts("");

    return (struct sylk_object){.type = SYLK_OBJ_NUMBER, .num_value = number};
};

static struct sylk_object to_string(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)self;
    (void)ctx;

    char* str = gc_alloc(vm, SYLK_OBJ_STRING, 200);

    struct sylk_object num = pop();

    sprintf(str, "%d", num.num_value);
    return (struct sylk_object){.type = SYLK_OBJ_STRING, .str_value = str};
}

static struct sylk_object to_int(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)self;
    (void)ctx;

    struct sylk_object str = pop();

    char* end = NULL;
    int32_t num = strtol(str.str_value, &end, 10);

    return (struct sylk_object){.type = SYLK_OBJ_NUMBER, .num_value = num};
}

static struct sylk_object input_string(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)self;
    (void)ctx;

    struct sylk_object o = pop();

    const char* input_text = o.str_value;
    printf("%s", input_text);

    char* string = gc_alloc(vm, SYLK_OBJ_STRING, 200);
    scanf("%s", string);

    puts("");

    return (struct sylk_object){.type = SYLK_OBJ_STRING, .str_value = string};
}

int add_builtin_functions(struct sylk_named_function* functions, size_t* n_functions) {
    functions[(*n_functions)++] = (struct sylk_named_function){.name = "print", (struct sylk_object_function){.type = SYLK_BUILT_IN, .n_parameters = 1, .function = print_object}};
    functions[(*n_functions)++] = (struct sylk_named_function){.name = "input_number", (struct sylk_object_function){.type = SYLK_BUILT_IN, .n_parameters = 1, .function = input_number}};
    functions[(*n_functions)++] = (struct sylk_named_function){.name = "input_string", (struct sylk_object_function){.type = SYLK_BUILT_IN, .n_parameters = 1, .function = input_string}};
    functions[(*n_functions)++] = (struct sylk_named_function){.name = "str", (struct sylk_object_function){.type = SYLK_BUILT_IN, .n_parameters = 1, .function = to_string}};
    functions[(*n_functions)++] = (struct sylk_named_function){.name = "int", (struct sylk_object_function){.type = SYLK_BUILT_IN, .n_parameters = 1, .function = to_int}};

    return 0;
};
