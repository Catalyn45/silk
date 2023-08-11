#include "gc.h"

#include <stdlib.h>
#include "objects.h"
#include "vm.h"

void* gc_alloc(struct vm* vm, size_t size) {
    void* item = malloc(size);
    if (!item)
        return NULL;

    struct gc* gc = &vm->gc;
    if (gc->n_items >= gc->treshold) {
        gc_clean(vm);
    }

    gc->pool[gc->n_items++] = (struct gc_item){
        .marked = false,
        .memory = item
    };

    return item;
}

static struct gc_item* get_item(struct gc* gc, void* item) {
    for (uint32_t i = 0; i < gc->n_items; ++i) {
        if (gc->pool[i].memory == item)
            return &gc->pool[i];
    }

    return NULL;
}

static void mark_item(struct gc* gc, struct object* obj) {
    switch (obj->type) {
        case OBJ_INSTANCE:
        {
            struct object_instance* instance = obj->instance_value;
            struct gc_item* item = get_item(gc, instance);
            if (item->marked)
                break;

            item->marked = true;
            if (instance->type == USER) {
                for (uint32_t i = 0; i < instance->class_index->n_members; ++i) {
                    mark_item(gc, &instance->members[i]);
                }
            } else if (instance->type == BUILT_IN) {
                for (uint32_t i = 0; i < instance->buintin_index->n_members; ++i) {
                    mark_item(gc, &instance->members[i]);
                }
            }
        }
        break;

        case OBJ_USER:
        {
            struct gc_item* item = get_item(gc, obj->user_value);
            item->marked = true;
        }
        break;
    }
}

void* gc_clean(struct vm* vm) {
    struct gc* gc = &vm->gc;
    for (uint32_t i = 0; i < vm->stack_size; ++i) {
        mark_item(gc, &vm->stack[i]);
    }

    uint32_t i = 0;

    while (i < gc->n_items) {
        while (i < gc->n_items && !gc->pool[i].marked) {
            free(gc->pool[i].memory);
            gc->pool[i] = gc->pool[--gc->n_items];
        }
        ++i;
    }

    return NULL;
}
