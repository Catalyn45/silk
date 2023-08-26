#ifndef OBJECTS_H_
#define OBJECTS_H_

#include <stdint.h>
#include <stdbool.h>

enum sylk_object_type {
    SYLK_OBJ_NUMBER   = 0,
    SYLK_OBJ_STRING   = 1,
    SYLK_OBJ_BOOL     = 2,
    SYLK_OBJ_USER     = 3,
    SYLK_OBJ_FUNCTION = 4,
    SYLK_OBJ_INSTANCE = 5,
    SYLK_OBJ_CLASS    = 6,
    SYLK_OBJ_COUNT    = 7
};

enum sylk_implementation_type {
    SYLK_USER     = 0,
    SYLK_BUILT_IN = 1
};

typedef void (*sylk_free_function)(void* mem);

struct sylk_object_user {
    void* mem;
    sylk_free_function free_fun;
};

struct sylk_object {
    int32_t type;

    union {
        int32_t     num_value;
        char*       str_value;
        bool        bool_value;
        void*       obj_value;
    };
};

struct sylk_vm;
typedef struct sylk_object (*builtin_fun)(struct sylk_object* self, struct sylk_vm* vm, void* ctx);

struct sylk_object_function {
    int32_t type;

    union {
        int32_t index;
        builtin_fun function;
    };

    int32_t n_parameters;
    struct sylk_object context;
};

struct sylk_named_function {
    const char* name;
    struct sylk_object_function function;
};

struct sylk_object_instance {
    struct sylk_object_class* cls;
    struct sylk_object members[256];
};

typedef void (*sylk_object_callback)(struct sylk_object* o, void* ctx);
typedef void (*sylk_iterate_function)(struct sylk_object_instance* self, sylk_object_callback cb, void* ctx);

struct sylk_object_class {
    int32_t type;

    uint32_t index;

    sylk_iterate_function iterate_fun;

    int32_t constructor;

    const char* members[10];
    uint32_t n_members;

    struct sylk_named_function methods[10];
    uint32_t n_methods;
};

struct sylk_named_class {
    const char* name;
    struct sylk_object_class cls;
};

#endif
