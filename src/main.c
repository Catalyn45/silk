#include "ast.h"
#include "lexer.h"
#include "utils.h"
#include "string.h"
#include "parser.h"
#include "vm.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: %s <file_name> [-a] [-h]\n", argv[0]);
        printf("help:\n");
        printf("\t<file_name> : file with code to execute\n");
        printf("\t-a          : dump the abstract syntax tree\n");
        printf("\t-h          : not execute the program (use with -a to only show the ast)\n");
        return 1;
    }

    const char * file_name = argv[1];
    FILE* f = fopen(file_name, "r");
    if (!f) {
        printf("%s not existent\n", file_name);
        return 1;
    }

    bool print_ast = argc > 2 && strcmp(argv[2], "-a") == 0;
    bool execute_program = argc <= 3 || strcmp(argv[3], "-h") != 0;

    char text[2048];
    size_t size = fread(text, sizeof(char), sizeof(text), f);
    fclose(f);

    text[size] = '\0';

    struct token_entry* tokens = NULL;
    uint32_t n_tokens;
    int res = tokenize(text, strlen(text), &tokens, &n_tokens);
    if (res != 0) {
        ERROR("failed to tokenize");
        goto free_tokens;
    }

    struct parser parser = {
        .text = text,
        .tokens = tokens,
        .n_tokens = n_tokens
    };
    struct node* ast = NULL;

    res = parse(&parser, &ast);
    if (res != 0) {
        ERROR("failed to parse");
        goto free_nodes;
    }

    if (print_ast) {
        puts("ast:");
        dump_ast(ast, 0, false);
    }

    if (execute_program) {
        uint8_t bytes[2048];
        uint32_t n_bytes = 0;

        struct evaluator e = {};
        struct binary_data d = {};
        if (evaluate(ast, bytes, &n_bytes, &d, &e) != 0) {
            ERROR("failed to evaluate")
            return 1;
        }

        disassembly(bytes, n_bytes);

        struct vm vm = {
            .bytes = bytes,
            .n_bytes = n_bytes,
        };

        if (execute(&vm) != 0) {
            ERROR("failed to execute");
            return 1;
        }
    }


free_tokens:
    for (uint32_t i = 0; i < n_tokens; ++i) {
        free(tokens[i].value);
    }

    free(tokens);

free_nodes:
    if (ast)
        node_free(ast);

    return res;
}
