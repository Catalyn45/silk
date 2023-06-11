#include "lexer.h"
#include "utils.h"
#include "string.h"
#include "parser.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    (void)argc;

    struct token_entry* tokens = NULL;
    uint32_t n_tokens;

    const char * file_name = argv[1];

    FILE* f = fopen(file_name, "r");
    if (!f) {
        printf("%s not existent\n", file_name);
        return 1;
    }

    char text[2048];

    size_t size = fread(text, sizeof(char), sizeof(text), f);
    text[size] = '\0';

    int res = tokenize(text, strlen(text), &tokens, &n_tokens);
    if (res != 0) {
        // TODO: error
        return 1;
    }

    struct node* ast = NULL;
    parse(tokens, n_tokens, &ast);


    puts("ast:");
    dump_ast(ast, 0);

    evaluate(ast);

    free(tokens);
    return 0;
}
