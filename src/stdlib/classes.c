#include <stdint.h>
#include <stdlib.h>

#include "classes.h"
#include "../vm.h"

struct list_context {
    struct object* container;
    uint32_t n_elements;
    uint32_t allocated;
};

#define INIT_SIZE 1

static void list_free(void* mem) {
    struct list_context* ctx = mem;

    free(ctx->container);
    free(ctx);
}

static struct object list_constructor(struct object* self, struct vm* vm) {
    (void)vm;

    struct object* container = malloc(INIT_SIZE * sizeof(*container));
    struct list_context* context = malloc(sizeof(*context));
    *context = (struct list_context) {
        .container = container,
        .allocated = INIT_SIZE
    };

    struct object_user* user = gc_alloc(vm, OBJ_USER, sizeof(*user));
    *user = (struct object_user) {
        .mem = context,
        .free_fun = list_free
    };

    struct object_instance* instance = self->obj_value;
    instance->members[0] = (struct object) {
        .type = OBJ_USER,
        .obj_value = user
    };

    return (struct object){};
}

static struct object list_add(struct object* self, struct vm* vm) {
    struct object_instance* instance = self->obj_value;
    struct object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    if (context->n_elements >= context->allocated) {
        uint32_t new_alloc_size = context->allocated * 2;

        struct object* new_mem = realloc(context->container, new_alloc_size * sizeof(*context->container));
        context->container = new_mem;
        context->allocated = new_alloc_size;
    }

    context->container[context->n_elements++] = peek(0);
    return (struct object){};
}

static struct object list_pop(struct object* self, struct vm* vm) {
    (void)vm;

    struct object_instance* instance = self->obj_value;
    struct object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    return context->container[--context->n_elements];
}

static void iterate_objects(struct object_instance* self, object_callback cb, void* ctx) {
    struct object_user* user = self->members[0].obj_value;
    struct list_context* context = user->mem;

    for (uint32_t i = 0; i < context->n_elements; ++i) {
        cb(&context->container[i], ctx);
    }
}

static struct object list_length(struct object* self, struct vm* vm) {
    (void)vm;

    struct object_instance* instance = self->obj_value;
    struct object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    return (struct object) {.type = OBJ_NUMBER, .num_value = context->n_elements};
}

static struct object list_set(struct object* self, struct vm* vm) {
    struct object_instance* instance = self->obj_value;
    struct object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    uint32_t index = peek(0).num_value;
    context->container[index] = peek(1);

    return (struct object){};
}

static struct object list_get(struct object* self, struct vm* vm) {
    struct object_instance* instance = self->obj_value;
    struct object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    uint32_t index = peek(0).num_value;
    return context->container[index];
}

int add_builtin_classes(struct evaluator* e) {
    struct named_class list = {
        .name = "list",
        .cls = (struct object_class) {
            .type = BUILT_IN,
            .index = 0,
            .iterate_fun = iterate_objects,
            .n_members = 1,
            .n_methods = 6,
            .constructor = 0,
            .methods = {
                {
                    .name = "constructor",
                    .function = {
                        .type = BUILT_IN,
                        .function = list_constructor,
                    }
                },
                {
                    .name = "add",
                    .function = {
                        .type = BUILT_IN,
                        .function = list_add,
                    }
                },
                {
                    .name = "pop",
                    .function = {
                        .type = BUILT_IN,
                        .function = list_pop
                    }
                },
                {
                    .name = "length",
                    .function = {
                        .type = BUILT_IN,
                        .function = list_length
                    }
                },
                {
                    .name = "__set",
                    .function = {
                        .type = BUILT_IN,
                        .function = list_set
                    }
                },
                {
                    .name = "__get",
                    .function = {
                        .type = BUILT_IN,
                        .function = list_get
                    }
                }
            }
        }
    };

    e->classes[e->n_classes++] = list;

    return 0;
}
