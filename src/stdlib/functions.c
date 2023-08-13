#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "../objects.h"
#include "../vm.h"


static struct object print_object(struct vm* vm) {
    struct object o = peek(0);

    if (o.type == OBJ_NUMBER) {
        printf("%d\n", o.num_value);
    } else if (o.type == OBJ_STRING) {
        printf("%s\n", o.str_value);
    }

    return (struct object){};
}

static struct object input_number(struct vm* vm) {
    struct object o = peek(0);

    const char* input_text = o.str_value;
    printf("%s", input_text);

    int32_t number;
    scanf("%d", &number);

    puts("");

    return (struct object){.type = OBJ_NUMBER, .num_value = number};
};

static struct object to_string(struct vm* vm) {
    char* str = gc_alloc(vm, 200);
    struct object num = peek(0);

    sprintf(str, "%d", num.num_value);
    return (struct object){.type = OBJ_STRING, .str_value = str};
}

static struct object to_int(struct vm* vm) {
    struct object str = peek(0);

    char* end = NULL;
    int32_t num = strtol(str.str_value, &end, 10);

    return (struct object){.type = OBJ_NUMBER, .num_value = num};
}

static struct object input_string(struct vm* vm) {
    struct object o = peek(0);

    const char* input_text = o.str_value;
    printf("%s", input_text);

    char* string = gc_alloc(vm, 200);
    scanf("%s", string);

    puts("");

    return (struct object){.type = OBJ_STRING, .str_value = string};
}

int add_builtin_functions(struct evaluator* e) {
    uint32_t index = 0;

    e->functions[e->n_functions++] = (struct function){.name = "print",        .n_parameters = 1, .fun = print_object, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "input_number", .n_parameters = 1, .fun = input_number, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "input_string", .n_parameters = 1, .fun = input_string, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "str", .n_parameters = 1, .fun = to_string, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "int", .n_parameters = 1, .fun = to_int, .index = index++};

    return 0;
};
