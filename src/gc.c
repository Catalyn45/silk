#include "gc.h"

#include <stdio.h>
#include <stdlib.h>
#include "objects.h"
#include "vm.h"

void* gc_alloc(struct sylk_vm* vm, int32_t type, size_t size) {
    void* item = malloc(size);
    if (!item)
        return NULL;

    struct gc* gc = &vm->gc;

    gc->pool[gc->n_items++] = (struct gc_item){
        .type = type,
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

static void mark_item(struct gc* gc, struct sylk_object* obj);

static void mark_item_cb(struct sylk_object* o, void* ctx) {
    struct gc* gc = ctx;
    mark_item(gc, o);
}

static void mark_item(struct gc* gc, struct sylk_object* obj) {
    struct gc_item* item = get_item(gc, obj->obj_value);
    if (!item)
        return;

    if (item->marked)
        return;

    item->marked = true;
    switch (obj->type) {
        case SYLK_OBJ_INSTANCE:
            {
                struct sylk_object_instance* instance = item->memory;
                struct sylk_object_class* cls = instance->cls;

                for (uint32_t i = 0; i < cls->n_members; ++i) {
                    mark_item(gc, &instance->members[i]);
                }

                if (cls->iterate_fun) {
                    cls->iterate_fun(instance, mark_item_cb, gc);
                }

            }
            break;
    }
}

void gc_clean(struct sylk_vm* vm) {
    struct gc* gc = &vm->gc;
    if (gc->n_items < gc->treshold)
        return;

    for (uint32_t i = 0; i < vm->stack_size; ++i) {
        mark_item(gc, &vm->stack[i]);
    }

    uint32_t i = 0;
    while (i < gc->n_items) {
        while (i < gc->n_items && !gc->pool[i].marked) {
            printf("deleting memory\n");

            struct gc_item* item = &gc->pool[i];
            if (item->type == SYLK_OBJ_USER) {
                struct sylk_object_user* user = item->memory;
                user->free_fun(user->mem);
            }

            free(item->memory);
            *item = gc->pool[--gc->n_items];
        }

        if (i < gc->n_items) {
            gc->pool[i].marked = false;
        }

        ++i;
    }
}
