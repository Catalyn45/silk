#include "lexer.h"
#include "string.h"
#include "parser.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
/*
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
*/
/*
static void print_node(const struct node* n) {
    switch (n->type) {
        case NODE_NUMBER:
            printf("NODE_NUMBER\n");
            break;
        case NODE_BINARY_OP:
            printf("NODE_BINARY_OP\n");
            break;
    }
}
static void print_ast(const struct node* ast, uint32_t indent) {
    if (ast == NULL) {
        return;
    }
    for (uint32_t i = 0; i < indent; i++) {
        printf("\t");
    }

    print_node(ast);
    print_ast(ast->left, indent + 1);
    print_ast(ast->right, indent + 2);
}
*/
int main(int argc, char* argv[]) {
    (void)argc;
    struct token_entry* tokens = NULL;
    uint32_t n_tokens;

    const char * text = argv[1];


    int res = tokenize(text, strlen(text), &tokens, &n_tokens);
    if (res != 0) {
        // TODO: error
        return 1;
    }
    /*
    for (uint32_t i = 0; i < n_tokens; i++) {
        print_tok(&tokens[i]);
    }
    */
    
    struct node* ast = NULL;
    parse(tokens, n_tokens, &ast);

    // print_ast(ast, 0);

    int result = evaluate(ast);
    printf("%d\n", result);

    free(tokens);
    return 0;
}
