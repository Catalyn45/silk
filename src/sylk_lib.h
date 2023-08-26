#ifndef _SYLK_LIB_H
#define _SYLK_LIB_H

#include "objects.h"
#include "stddef.h"

struct sylk {
    struct sylk_named_function builtin_functions[1024];
    size_t n_builtin_functions;

    struct sylk_named_class builtin_classes[1024];
    size_t n_builtin_classes;

    const struct sylk_config* config;

    void* ctx;
};

#endif
