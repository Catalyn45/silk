#include <stdio.h>
#include "parser.h"
#include "utils.h"

static const char* rev_tokens[] = {
    "int",       // TOK_INT, // integer (20)
    "str",       // TOK_STR, // string ("example")
    "+",         // TOK_ADD, // add operator (+)
    "-",         // TOK_MIN, // minux operator (-)
    "*",         // TOK_MUL, // multiply operator (*)
    "/",         // TOK_DIV, // division operator (/)
    "=",         // TOK_EQL, // equal (=)
    "(",         // TOK_LPR, // left parantesis (()
    ")",         // TOK_RPR, // right parantesis ())
    "[",         // TOK_LSQ, // left square parantesis ([)
    "]",         // TOK_RSQ, // right square parantesis (])
    "{",         // TOK_LBR, // left bracket ({)
    "}",         // TOK_RBR, // right bracket (})
    [18] = "<",
    "<=",
    ">",
    "<=",
    "==",
    "!"
};

static const char* rev_node[] = {
    "NUMBER",
    "BINARY_OP",
    "IF",
    "WHILE",
    "ASSIGN",
    "STATEMENT",
    "VARIABLE",
    "FUNCTION_CALL",
    "NOT"
};

void dump_ast(struct node* root, int indent) {
    if (!root)
        return;

    for (int i = 0; i < indent - 4; ++i)
        printf(" ");

    if (indent > 0)
        printf("└── ");

    printf("%s", rev_node[root->type]);

    if (root->type == NODE_NUMBER) {
        printf("(%d)\n", *(int*)root->token->token_value);
    } else if (root->token) {
        if (root->token->token_value) {
            printf("(%s)\n", (char*)root->token->token_value);
        } else {
            printf("(%s)\n", rev_tokens[root->token->token_code]);
        }
    } else {
        printf("\n");
    }

    if (root->type == NODE_STATEMENT) {
        dump_ast(root->value, indent+4);
        dump_ast(root->left, indent);
        return;
    }

    dump_ast(root->value, indent+4);
    dump_ast(root->left, indent+4);
    dump_ast(root->right, indent+4);
}

