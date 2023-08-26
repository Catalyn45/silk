#ifndef GC_H_
#define GC_H_

#include <stdbool.h>
#include <stdlib.h>

struct gc_item {
    bool marked;
    int32_t type;
    void* memory;
};

struct gc {
    struct gc_item pool[300];
    size_t n_items;
    size_t treshold;
};

struct sylk_vm;

void* gc_alloc(struct sylk_vm* vm, int32_t type, size_t size);
void gc_clean(struct sylk_vm* vm);

#endif
