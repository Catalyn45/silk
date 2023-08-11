#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "../objects.h"
#include "../vm.h"


static struct object print_object(struct vm* vm) {
    struct object o = peek(0);

    if (o.type == OBJ_NUMBER) {
        printf("%d\n", o.int_value);
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

    return (struct object){.type = OBJ_NUMBER, .int_value = number};
};

static struct object input_string(struct vm* vm) {
    struct object o = peek(0);

    const char* input_text = o.str_value;
    printf("%s", input_text);

    char* string = malloc(200);
    scanf("%s", string);

    puts("");

    return (struct object){.type = OBJ_STRING, .str_value = string};
}

int add_builtin_functions(struct evaluator* e) {
    uint32_t index = 0;

    e->functions[e->n_functions++] = (struct function){.name = "print",        .n_parameters = 1, .fun = print_object, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "input_number", .n_parameters = 1, .fun = input_number, .index = index++};
    e->functions[e->n_functions++] = (struct function){.name = "input_string", .n_parameters = 1, .fun = input_string, .index = index++};

    return 0;
};
