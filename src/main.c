#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

static void print_tok(const struct token_entry* token) {
    switch (token->token_code) {
        case TOK_INT:
            printf("TOK_INT(%d)\n", *((int*)token->token_value));
            break;
        case TOK_STR:
            printf("TOK_STR(%s)\n", (const char*)token->token_value);
            break;
        case TOK_ADD:
            printf("TOK_ADD\n");
            break;
        case TOK_MIN:
            printf("TOK_MIN\n");
            break;
        case TOK_MUL:
            printf("TOK_MUL\n");
            break;
        case TOK_DIV:
            printf("TOK_DIV\n");
            break;
        case TOK_LPR:
            printf("TOK_LPR\n");
            break;
        case TOK_RPR:
            printf("TOK_RPR\n");
            break;
        case TOK_EQL:
            printf("TOK_EQL\n");
            break;
        case TOK_EOF:
            printf("TOK_EOF\n");
            break;
    }
}

int main() {
    struct token_entry* tokens = NULL;
    uint32_t n_tokens;

    const char text[] = "(1 + 2) / 3 * 5 - 'alabala' + \"portocala\" = 3";

    int res = tokenize(text, sizeof(text) - 1, &tokens, &n_tokens);
    if (res != 0) {
        // TODO: error
        return 1;
    }

    for (uint32_t i = 0; i < n_tokens; i++) {
        print_tok(&tokens[i]);
    }

    free(tokens);
    return 0;
}
