#ifndef OBJECTS_H_
#define OBJECTS_H_

#include <stdint.h>
#include <stdbool.h>

enum object_type {
    OBJ_NUMBER   = 0,
    OBJ_STRING   = 1,
    OBJ_BOOL     = 2,
    OBJ_USER     = 3,
    OBJ_FUNCTION = 4,
    OBJ_INSTANCE = 5,
    OBJ_CLASS    = 6,
    OBJ_COUNT    = 7
};

enum implementation_type {
    USER     = 0,
    BUILT_IN = 1
};


typedef void (*free_function)(void* mem);
struct object_user {
    void* mem;
    free_function free_fun;
};


struct object {
    int32_t type;

    union {
        int32_t     num_value;
        char*       str_value;
        bool        bool_value;
        void*       obj_value;
    };
};

struct vm;
typedef struct object (*builtin_fun)(struct object* self, struct vm* vm);

struct object_function {
    int32_t type;

    union {
        int32_t index;
        builtin_fun function;
    };

    int32_t n_parameters;
    struct object context;
};

struct named_function {
    const char* name;
    struct object_function function;
};

struct object_instance {
    struct object_class* cls;
    struct object members[256];
};

typedef void (*object_callback)(struct object* o, void* ctx);
typedef void (*iterate_function)(struct object_instance* self, object_callback cb, void* ctx);

struct object_class {
    int32_t type;

    uint32_t index;

    iterate_function iterate_fun;

    int32_t constructor;

    const char* members[10];
    uint32_t n_members;

    struct named_function methods[10];
    uint32_t n_methods;
};

struct named_class {
    const char* name;
    struct object_class cls;
};

#endif
