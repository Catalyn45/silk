#include <stdio.h>
#include "parser.h"
#include "utils.h"

const char* rev_tokens[] = {
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
    "eof",       // TOK_EOF  // end of file
    "if",        // TOK_IF   // if
    "else",      // TOK_ELS  // else
    "while",     // TOK_WHL // while
    "identifier",// TOK_IDN // identifier
    "<",
    "<=",
    ">",
    "<=",
    "==",
    "!=",
    "!",
    "and",
    "or",
    ",",
    "def"
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
    "NOT",
    "FUNCTION",
    "DECISION"
};

void dump_ast(struct node* root, int indent, bool is_statement) {
    if (!root)
        return;

    for (int i = 0; i < indent - 4; ++i)
        printf(" ");

    if (indent > 0) {
        if (is_statement) {
            printf("    ");
        } else {
            printf("└── ");
        }
    }

    printf("%s", rev_node[root->type]);

    if (root->type == NODE_NUMBER) {
        printf("(%d)\n", *(int*)root->token->value);
    } else if (root->token) {
        if (root->token->value) {
            printf("(%s)\n", (char*)root->token->value);
        } else {
            printf("(%s)\n", rev_tokens[root->token->code]);
        }
    } else {
        printf("\n");
    }

    if (root->type == NODE_STATEMENT) {
        dump_ast(root->left, indent+4, false);
        dump_ast(root->right, indent, true);
        return;
    }

    dump_ast(root->left, indent+4, false);
    dump_ast(root->right, indent+4, false);
}

#define N_POSITION 10
#define min(a, b) ((a) < (b) ? a : b)
#define max(a, b) ((a) > (b) ? a : b)


void print_program_error(const char* text, uint32_t index) {
    uint32_t i = max(0, index - N_POSITION);
    uint32_t start_index = index;

    while (start_index >= i && text[start_index] != '\n')
        --start_index;

    i = start_index + 1;

    printf("[ERROR] ");
    while (text[i] && text[i] != '\n' && text[i] && i < index + N_POSITION) {
        printf("%c", text[i]);
        ++i;
    }
    printf("\n");

    i = start_index + 1;

    printf("[ERROR] ");
    while (text[i] && text[i] != '\n' && text[i] && i < index + N_POSITION) {
        if (i == index) {
            printf("^");
        } else {
            printf(" ");
        }
        ++i;
    }
    printf("\n");
}
