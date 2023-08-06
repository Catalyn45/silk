#include <stdio.h>
#include "parser.h"
#include "utils.h"
#include "vm.h"

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
    "def",
    "return",
    "true",
    "false",
    ".",
    "var",
    "class"
};

static const char* rev_node[] = {
    "NUMBER",
    "STRING",
    "BINARY_OP",
    "IF",
    "WHILE",
    "ASSIGN",
    "STATEMENT",
    "VARIABLE",
    "FUNCTION_CALL",
    "NOT",
    "FUNCTION",
    "DECISION",
    "PARAMETER",
    "ARGUMENT",
    "RETURN",
    "EXPRESSION_STMT",
    "BOOL",
    "MEMBER_ACCESS",
    "INDEX",
    "DECLARATION",
    "METHOD",
    "MEMBER",
    "CLASS",
    "BLOCK",
    "METHOD_FUN"
};

static const char* rev_instruction[] = {
    "PUSH",
    "PUSH_TRUE",
    "PUSH_FALSE",
    "POP",
    "ADD",
    "MIN",
    "MUL",
    "DIV",
    "NOT",
    "DEQ",
    "NEQ",
    "GRE",
    "GRQ",
    "LES",
    "LEQ",
    "AND",
    "OR",
    "DUP",
    "DUP_LOC",
    "DUP_REG",
    "CHANGE",
    "CHANGE_REG",
    "CHANGE_LOC",
    "JMP_NOT",
    "JMP",
    "CALL",
    "RET",
    "PUSH_NUM",
    "GET_FIELD",
    "SET_FIELD"
};

void disassembly(const uint8_t* bytes, uint32_t n_bytes, uint32_t start_address) {
    for (uint32_t i = start_address; i < n_bytes; ++i) {
        printf("%-3d : %s", i, rev_instruction[bytes[i]]);

        switch (bytes[i]) {
            case PUSH:
            case PUSH_NUM:
            case DUP:
            case DUP_LOC:
            case DUP_REG:
            case CHANGE_REG:
                printf(" %d", *((uint32_t*)&bytes[i + 1]));
                i += sizeof(uint32_t);
                break;

            case JMP_NOT:
            case JMP:
                printf(" %d", start_address + *((uint32_t*)&bytes[i + 1]));
                i += sizeof(uint32_t);
                break;
        }

        printf("\n");
    }
}

void dump_ast(struct node* root, int indent) {
    if (!root)
        return;

    if (root->type == NODE_STATEMENT || root->type == NODE_PARAMETER || root->type == NODE_ARGUMENT || root->type == NODE_MEMBER) {
        dump_ast(root->right, indent);
    }

    for (int i = 0; i < indent - 4; ++i)
        printf(" ");

    if (indent > 0) {
        printf("└── ");
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

    dump_ast(root->left, indent+4);

    if (root->type != NODE_STATEMENT && root->type != NODE_PARAMETER && root->type != NODE_ARGUMENT && root->type != NODE_MEMBER) {
        dump_ast(root->right, indent+4);
    }
}

#define N_POSITION 30
#define min(a, b) ((a) < (b) ? a : b)
#define max(a, b) ((a) > (b) ? a : b)


void print_program_error(const char* text, int32_t index) {
    int32_t i = max(0, index - N_POSITION);
    int32_t start_index = index;

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
