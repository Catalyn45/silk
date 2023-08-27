#ifndef SYLK_H_
#define SYLK_H_

#include <stddef.h>
#include <stdbool.h>
#include "objects.h"


struct sylk_config {
    bool print_ast;
    bool print_bytecode;
    bool halt_program;
};


struct sylk_function {
    const char* name;
    builtin_fun function;
    size_t n_parameters;
};


struct sylk_class {
    const char* name;

    const char** members;
    struct sylk_function* methods;
};


struct sylk;


/**
 * create a new interpreter instance
 *
 * @param config interpreter configuration
 * @param ctx user provided context propagated to all functions
 *
 * @return interpreter instance
 */
struct sylk* sylk_new(const struct sylk_config* config, void* ctx);


/**
* free interpreter resources
*
* @param s sylk instance
*/
void sylk_free(struct sylk* s);


/**
 * load all of the default functions and classes like "list" and "print"
 *
 * @param s sylk instance
 *
 * @return success code
 */
int sylk_load_prelude(struct sylk* s);


/**
 * load user defined functions to sylk interpreter
 *
 * @param s interpreter instance
 * @param functions user functions to load in prelude (last element should contain NULL and 0)
 *
 * @return success code
 */
int sylk_load_functions(struct sylk* s, struct sylk_function* functions);


/**
 * load user defined classes to sylk interpreter
 *
 * @param s interpreter instance
 * @param classes user classes to load in prelude (last element should contain NULL and 0)
 *
 * @return success code
 */
int sylk_load_classes(struct sylk* s, struct sylk_class* classes);


/**
 * run program stored in string
 *
 * @param s interpreter instance
 * @param program program code to run
 * @param program_size length of program
 *
 * @return success code
 */
int sylk_run_string(struct sylk* s, const char* program, size_t program_size);


/**
 * run program from file
 *
 * @param s interpreter instance
 * @file_name file to run
 *
 * @return success code
 */
int sylk_run_file(struct sylk* s, const char* file_name);


/**
 * virtual machine instance
 */
struct sylk_vm;


/**
 * push object on the stack
 *
 * @param vm virtual machine instance
 * @param o object to push
 *
 * @return success code
 */
int sylk_push(struct sylk_vm* vm, const struct sylk_object* o);


/**
 * pop an object from the top of the stack
 *
 * @param vm virtual machine instance
 * @param o where the object will be stored
 *
 * @return success code
 */
int sylk_pop(struct sylk_vm* vm, struct sylk_object* o);


/**
 * peek an object from the top of the stack without popping
 *
 * @param vm virtual machine instance
 * @param o where the object will be stored
 *
 * @return success code
 */
int sylk_peek(struct sylk_vm* vm, size_t n, struct sylk_object* o);

#endif
