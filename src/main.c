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
    bool execute = argc <= 3 || strcmp(argv[3], "-h") != 0;

    char text[2048];
    size_t size = fread(text, sizeof(char), sizeof(text), f);
    fclose(f);

    text[size] = '\0';

    struct token_entry* tokens = NULL;
    uint32_t n_tokens;
    int res = tokenize(text, strlen(text), &tokens, &n_tokens);
    if (res != 0) {
        // TODO: error
        return 1;
    }

    struct node* ast = NULL;
    parse(tokens, n_tokens, &ast);

    if (print_ast) {
        puts("ast:");
        dump_ast(ast, 0);
    }

    if (execute) {
        evaluate(ast);
    }

    for (uint32_t i = 0; i < n_tokens; ++i) {
        free(tokens[i].token_value);
    }
    free(tokens);
    node_free(ast);

    return 0;
}
