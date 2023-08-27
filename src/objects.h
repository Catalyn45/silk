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


/**
 * functions and classes can have two types of implementation,
 * either builtin_in in interpreter or in code
 */
enum sylk_implementation_type {
    SYLK_USER     = 0,
    SYLK_BUILT_IN = 1
};


/**
 * function type for deleting an user object
 *
 * @param mem pointer to memory
 */
typedef void (*sylk_free_function)(void* mem);


/**
 * @field mem memory pointer
 * @field free_fun callback function to free the object
 */
struct sylk_object_user {
    void* mem;
    sylk_free_function free_fun;
};


/**
 * @field type object type, should be a value of sylk_object_type
 * @field num_value object value if object is number
 * @field str_value object value if object is string
 * @field str_value object value if object is string
 * @field obj_value object value if object is an head object
 */
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


/**
 * function definition for an user defined function
 *
 * @param self instance reference
 * @param vm sylk virtual machine instance
 * @param ctx user provided context in "sylk_new"
 *
 * @return function return value to pe pushed on the stack
 */
typedef struct sylk_object (*builtin_fun)(struct sylk_object* self, struct sylk_vm* vm, void* ctx);


/**
 * structure representing a function object
 *
 * @field type function type, can be SYLK_BUILTIN or SYLK_USER
 * @field index index in the bytecodes of the function if function is SYLK_USER
 * @field function the function callback if the function is SYLK_BUILT_IN
 * @field n_parameters function parameters number
 * @field context in case the function is a method, context store the instance
 */
struct sylk_object_function {
    int32_t type;

    union {
        int32_t index;
        builtin_fun function;
    };

    int32_t n_parameters;
    struct sylk_object context;
};


/**
 * @field name function name
 * @field function function object
 */
struct sylk_named_function {
    const char* name;
    struct sylk_object_function function;
};


/**
 * structure representing an instance object
 *
 * @field cls class object
 * @field members instance members
 */
struct sylk_object_instance {
    struct sylk_object_class* cls;
    struct sylk_object members[256];
};


typedef void (*sylk_object_callback)(struct sylk_object* o, void* ctx);


/**
 * callback provided by used to iterate through all of the referred objects
 * of the user implemented class
 *
 * @param self instance reference
 * @param cb callback to pe called with every object
 * @param ctx context to be passed to callback
*/
typedef void (*sylk_iterate_function)(struct sylk_object_instance* self, sylk_object_callback cb, void* ctx);


/**
 * structure representing a class object
 *
 * @field type class type, can be one of SYLK_USER or SYLK_BUILT_IN
 * @field index bytecode index if the class is SYLK_USER
 * @field iterate_fun function callback to iterate through all of the references
 * @field constructor index of the method used as a constructor
 * @field members names of the members of the class
 * @field n_members count
 * @field class methods
 * @field methods count
 */
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


/**
 * @field name class name
 * @field cls class object
 */
struct sylk_named_class {
    const char* name;
    struct sylk_object_class cls;
};

#endif
