#include <stdint.h>
#include <stdlib.h>

#include "classes.h"
#include "../vm.h"

struct list_context {
    struct sylk_object* container;
    uint32_t n_elements;
    uint32_t allocated;
};

#define INIT_SIZE 1

static void list_free(void* mem) {
    struct list_context* ctx = mem;

    free(ctx->container);
    free(ctx);
}

static struct sylk_object list_constructor(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)vm;
    (void)ctx;

    struct sylk_object* container = malloc(INIT_SIZE * sizeof(*container));
    struct list_context* context = malloc(sizeof(*context));
    *context = (struct list_context) {
        .container = container,
        .allocated = INIT_SIZE
    };

    struct sylk_object_user* user = gc_alloc(vm, SYLK_OBJ_NUMBER, sizeof(*user));
    *user = (struct sylk_object_user) {
        .mem = context,
        .free_fun = list_free
    };

    struct sylk_object_instance* instance = self->obj_value;
    instance->members[0] = (struct sylk_object) {
        .type = SYLK_OBJ_NUMBER,
        .obj_value = user
    };

    return (struct sylk_object){};
}

static struct sylk_object list_add(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)ctx;

    struct sylk_object_instance* instance = self->obj_value;
    struct sylk_object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    if (context->n_elements >= context->allocated) {
        uint32_t new_alloc_size = context->allocated * 2;

        struct sylk_object* new_mem = realloc(context->container, new_alloc_size * sizeof(*context->container));
        context->container = new_mem;
        context->allocated = new_alloc_size;
    }

    context->container[context->n_elements++] = peek(0);
    return (struct sylk_object){};
}

static struct sylk_object list_pop(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)vm;
    (void)ctx;

    struct sylk_object_instance* instance = self->obj_value;
    struct sylk_object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    return context->container[--context->n_elements];
}

static void iterate_objects(struct sylk_object_instance* self, sylk_object_callback cb, void* ctx) {
    struct sylk_object_user* user = self->members[0].obj_value;
    struct list_context* context = user->mem;

    for (uint32_t i = 0; i < context->n_elements; ++i) {
        cb(&context->container[i], ctx);
    }
}

static struct sylk_object list_length(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)vm;
    (void)ctx;

    struct sylk_object_instance* instance = self->obj_value;
    struct sylk_object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    return (struct sylk_object) {.type = SYLK_OBJ_NUMBER, .num_value = context->n_elements};
}

static struct sylk_object list_set(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)ctx;

    struct sylk_object_instance* instance = self->obj_value;
    struct sylk_object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    uint32_t index = peek(0).num_value;
    context->container[index] = peek(1);

    return (struct sylk_object){};
}

static struct sylk_object list_get(struct sylk_object* self, struct sylk_vm* vm, void* ctx) {
    (void)ctx;

    struct sylk_object_instance* instance = self->obj_value;
    struct sylk_object_user* user = instance->members[0].obj_value;
    struct list_context* context = user->mem;

    uint32_t index = peek(0).num_value;
    return context->container[index];
}

int add_builtin_classes(struct sylk_named_class* classes, size_t* n_classes){
    struct sylk_named_class list = {
        .name = "list",
        .cls = (struct sylk_object_class) {
            .type = SYLK_BUILT_IN,
            .index = 0,
            .iterate_fun = iterate_objects,
            .n_members = 1,
            .n_methods = 6,
            .constructor = 0,
            .methods = {
                {
                    .name = "constructor",
                    .function = {
                        .type = SYLK_BUILT_IN,
                        .function = list_constructor,
                    }
                },
                {
                    .name = "add",
                    .function = {
                        .type = SYLK_BUILT_IN,
                        .function = list_add,
                    }
                },
                {
                    .name = "pop",
                    .function = {
                        .type = SYLK_BUILT_IN,
                        .function = list_pop
                    }
                },
                {
                    .name = "length",
                    .function = {
                        .type = SYLK_BUILT_IN,
                        .function = list_length
                    }
                },
                {
                    .name = "__set",
                    .function = {
                        .type = SYLK_BUILT_IN,
                        .function = list_set
                    }
                },
                {
                    .name = "__get",
                    .function = {
                        .type = SYLK_BUILT_IN,
                        .function = list_get
                    }
                }
            }
        }
    };

    classes[(*n_classes)++] = list;

    return 0;
}
