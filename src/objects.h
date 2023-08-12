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
    OBJ_METHOD   = 5,
    OBJ_INSTANCE = 6,
    OBJ_CLASS    = 7
};

enum implementation_type {
    USER     = 0,
    BUILT_IN = 1
};

struct object_function {
    int32_t type;

    int32_t index;
    int32_t n_parameters;
};

struct object {
    int32_t type;

    union {
        int32_t     num_value;
        const char* str_value;
        bool        bool_value;
        void*       user_value;
        struct object_function* function_value;
        struct object_method*   method_value;
        struct object_instance* instance_value;
        struct object_class*    class_value;
    };
};

struct object_method {
    int32_t type;

    int32_t index;
    int32_t n_parameters;

    struct object context;
};

struct object_instance {
    int32_t type;

    union {
        struct object_class* class_index;
        struct class_*       buintin_index;
    };

    struct object members[256];
    void* context;
};

struct pair {
    const char* name;
    int32_t index;
    uint32_t n_parameters;
};

struct object_class {
    int32_t type;
    uint32_t index;

    const char* name;

    int32_t constructor;

    const char* members[10];
    uint32_t n_members;

    struct pair methods[10];
    uint32_t n_methods;
};

#endif
