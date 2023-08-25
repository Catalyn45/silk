#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "../objects.h"

#include "../vm.h"

static struct object print_object(struct object* self, struct vm* vm) {
    (void)self;

    struct object o = peek(0);

    if (o.type == OBJ_NUMBER) {
        printf("%d\n", o.num_value);
    } else if (o.type == OBJ_STRING) {
        printf("%s\n", o.str_value);
    } else if (o.type == OBJ_BOOL) {
        printf("%s\n", o.bool_value ? "true" : "false");
    } else if (o.type == OBJ_CLASS) {
        printf("class\n");
    } else if (o.type == OBJ_INSTANCE) {
        printf("instance\n");
    } else if (o.type == OBJ_FUNCTION) {
        puts("function");
    } else if (o.type == OBJ_USER) {
        puts("user");
    }

    return (struct object){};
}

static struct object input_number(struct object* self, struct vm* vm) {
    (void)self;

    struct object o = peek(0);

    const char* input_text = o.str_value;
    printf("%s", input_text);

    int32_t number;
    scanf("%d", &number);

    puts("");

    return (struct object){.type = OBJ_NUMBER, .num_value = number};
};

static struct object to_string(struct object* self, struct vm* vm) {
    (void)self;

    char* str = gc_alloc(vm, OBJ_STRING, 200);

    struct object num = peek(0);

    sprintf(str, "%d", num.num_value);
    return (struct object){.type = OBJ_STRING, .str_value = str};
}

static struct object to_int(struct object* self, struct vm* vm) {
    (void)self;

    struct object str = peek(0);

    char* end = NULL;
    int32_t num = strtol(str.str_value, &end, 10);

    return (struct object){.type = OBJ_NUMBER, .num_value = num};
}

static struct object input_string(struct object* self, struct vm* vm) {
    (void)self;

    struct object o = peek(0);

    const char* input_text = o.str_value;
    printf("%s", input_text);

    char* string = gc_alloc(vm, OBJ_STRING, 200);
    scanf("%s", string);

    puts("");

    return (struct object){.type = OBJ_STRING, .str_value = string};
}

int add_builtin_functions(struct named_function* functions, size_t* n_functions) {
    functions[(*n_functions)++] = (struct named_function){.name = "print", (struct object_function){.type = BUILT_IN, .n_parameters = 1, .function = print_object}};
    functions[(*n_functions)++] = (struct named_function){.name = "input_number", (struct object_function){.type = BUILT_IN, .n_parameters = 1, .function = input_number}};
    functions[(*n_functions)++] = (struct named_function){.name = "input_string", (struct object_function){.type = BUILT_IN, .n_parameters = 1, .function = input_string}};
    functions[(*n_functions)++] = (struct named_function){.name = "str", (struct object_function){.type = BUILT_IN, .n_parameters = 1, .function = to_string}};
    functions[(*n_functions)++] = (struct named_function){.name = "int", (struct object_function){.type = BUILT_IN, .n_parameters = 1, .function = to_int}};

    return 0;
};
