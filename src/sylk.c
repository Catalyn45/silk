#include "sylk.h"

#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "objects.h"
#include "parser.h"
#include "sylk_lib.h"
#include "stdlib/functions.h"
#include "stdlib/classes.h"
#include "vm.h"
#include "utils.h"


struct sylk* sylk_new(const struct sylk_config* config, void* ctx) {
    struct sylk* s = malloc(sizeof(*s));
    if (!s) {
        return NULL;
    }

    *s = (struct sylk) {
        .config = config,
        .ctx = ctx
    };

    return s;
}

void sylk_free(struct sylk* s) {
    free(s);
}

int sylk_run_string(struct sylk* s, const char* program, size_t program_size) {
    struct lexer l = {
        .text = program,
        .text_size = program_size,
        .line = 1
    };

    struct parser parser = {
        .l = &l
    };

    struct node* ast = NULL;

    int res = parse(&parser, &ast);
    if (res != 0) {
        ERROR("failed to parse");

        const struct token* token = &parser.current_token;
        ERROR("at line %d", token->line);
        print_program_error(program, token->index);

        return 1;
    }

    if (s->config->print_ast) {
        dump_ast(ast, 0);
        puts("");
    }

    uint8_t bytecode[4096];
    size_t n_bytecodes = 0;
    uint32_t start_address;

    CHECK(compile_program(s, ast, bytecode, &n_bytecodes, &start_address), "failed to compile program");
    if (s->config->print_bytecode) {
        disassembly(bytecode, n_bytecodes, start_address);
        puts("");
    }

    if (!s->config->halt_program) {
        struct sylk_vm vm = {
            .bytes = bytecode,
            .n_bytes = n_bytecodes,
            .start_address = start_address
        };

        if (execute(s, &vm) != 0) {
            ERROR("failed to execute");
            return 1;
        }
    }

    if (ast)
        node_free(ast);

    return 0;
}

int sylk_run_file(struct sylk* s, const char* file_name) {
    FILE* f = fopen(file_name, "r");
    if (!f) {
        printf("%s not existent\n", file_name);
        return 1;
    }

    char text[2048];
    size_t size = fread(text, sizeof(char), sizeof(text), f);
    fclose(f);

    CHECK(sylk_run_string(s, text, size), "failed to run program");
    return 0;
}

int sylk_load_functions(struct sylk* s, struct sylk_function* functions) {
    size_t i = 0;
    while (functions[i].name && functions[i].function) {
        s->builtin_functions[s->n_builtin_functions++] = (struct sylk_named_function){.name = functions[i].name, (struct sylk_object_function){.type = SYLK_BUILT_IN, .n_parameters = functions[i].n_parameters, .function = functions[i].function}};
        ++i;
    }

    return 0;
}

int sylk_load_classes(struct sylk* s, struct sylk_class* classes) {
    size_t i = 0;
    while (classes[i].name && classes[i].members) {
        struct sylk_named_class cls = {
            .name = classes[i].name,
            .cls = (struct sylk_object_class) {
                .type = SYLK_BUILT_IN,
                .constructor = -1
            }
        };

        struct sylk_object_class* obj_cls = &cls.cls;

        size_t j = 0;
        while (classes[i].members[j]) {
            obj_cls->members[obj_cls->n_members++] = classes[i].members[j];
        }

        j = 0;
        while (classes[i].methods[j].name && classes[i].methods[j].function) {
            if (strcmp(classes[i].methods[j].name, "constructor") == 0) {
                obj_cls->constructor = j;
            }

            obj_cls->methods[obj_cls->n_methods++] = (struct sylk_named_function) {
                .name = classes[i].methods[j].name,
                .function = (struct sylk_object_function) {
                    .type = SYLK_BUILT_IN,
                    .n_parameters = classes[i].methods[j].n_parameters,
                    .function = classes[i].methods[j].function
                }
            };
            ++j;
        }

        s->builtin_classes[s->n_builtin_classes++] = cls;
        ++i;
    }

    return 0;
}

int sylk_push(struct sylk_vm* vm, const struct sylk_object* o){ 
    push(*o);
    return 0;
}

int sylk_pop(struct sylk_vm* vm, struct sylk_object* o){ 
    *o = pop();
    return 0;
}

int sylk_peek(struct sylk_vm* vm, size_t n, struct sylk_object* o){ 
    *o = peek(n);
    return 0;
}

int sylk_load_prelude(struct sylk* s) {
    add_builtin_functions(s->builtin_functions, &s->n_builtin_functions);
    add_builtin_classes(s->builtin_classes, &s->n_builtin_classes);

    return 0;
}

