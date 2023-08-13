#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "operations.h"
#include "objects.h"
#include "vm.h"

static int add_strings(struct vm* vm, struct object* op1, struct object* op2, struct object* result) {
    if (op2->type != OBJ_STRING) {
        ERROR("can't add objects of type %d and %d", op1->type, op2->type);
        return 1;
    }

    char* new_string = gc_alloc(vm, 500);
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
    if (op2->type != OBJ_NUMBER) {
        ERROR("can't add objects of type %d and %d", op1->type, op2->type);
        return 1;
    }


    *result = (struct object){.type = OBJ_NUMBER, .num_value = op1->num_value + op2->num_value};
    return 0;
}

operation_fun addition_table[OBJ_COUNT] = {
    [OBJ_STRING] = add_strings,
    [OBJ_NUMBER] = add_numbers
};
