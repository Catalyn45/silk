#include <stdint.h>
#include <stdlib.h>

#include "classes.h"
#include "../vm.h"

struct list_context {
    struct object container[20];
    uint32_t n_elements;
};

static struct object list_constructor(struct object self, struct vm* vm) {
    struct list_context* context = gc_alloc(vm, sizeof(*context));
    *context = (struct list_context) {};
    self.instance_value->context = context;

    return (struct object){};
}

static struct object list_add(struct object self, struct vm* vm) {
    struct list_context* context = self.instance_value->context;
    context->container[context->n_elements++] = peek(0);

    return (struct object){};
}

static struct object list_pop(struct object self, struct vm* vm) {
    (void)vm;
    struct list_context* context = self.instance_value->context;

    return context->container[--context->n_elements];
}

static struct object list_set(struct object self, struct vm* vm) {
    struct list_context* context = self.instance_value->context;

    uint32_t index = peek(0).int_value;
    context->container[index] = peek(1);

    return (struct object){};
}

static struct object list_get(struct object self, struct vm* vm) {
    struct list_context* context = self.instance_value->context;

    uint32_t index = peek(0).int_value;
    return context->container[index];
}

int add_builtin_classes(struct evaluator* e) {
    struct class_ list = {
        .name = "list",
        .index = 0,
        .n_methods = 5,
        .methods = {
            {
                .name = "constructor",
                .method = list_constructor
            },
            {
                .name = "add",
                .method = list_add
            },
            {
                .name = "pop",
                .method = list_pop
            },
            {
                .name = "__set",
                .method = list_set
            },
            {
                .name = "__get",
                .method = list_get
            }
        }
    };

    e->classes[e->n_classes++] = list;

    return 0;
}
