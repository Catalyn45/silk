#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>
#include <stdint.h>

enum token_codes {
    TOK_INV = 0, // invalid token
    TOK_INT = 1, // integer (20)
    TOK_STR = 2, // string ("example")
    TOK_ADD = 3, // add operator (+)
    TOK_MIN = 4, // minux operator (-)
    TOK_MUL = 5, // multiply operator (*)
    TOK_DIV = 6, // division operator (/)
    TOK_EQL = 7, // equal (=)
    TOK_LPR = 8, // left parantesis (()
    TOK_RPR = 9, // right parantesis ())
    TOK_LSQ = 10, // left square parantesis ([)
    TOK_RSQ = 11, // right square parantesis (])
    TOK_LBR = 12, // left bracket ({)
    TOK_RBR = 13, // right bracket (})
    TOK_EOF = 14, // end of file
    TOK_IF  = 15,  // if
    TOK_ELS = 16, // else
    TOK_WHL = 17, // while
    TOK_IDN = 18, // identifier
    TOK_LES = 19,
    TOK_LEQ = 20,
    TOK_GRE = 21,
    TOK_GRQ = 22,
    TOK_DEQ = 23,
    TOK_NEQ = 24,
    TOK_NOT = 25,
    TOK_AND = 26,
    TOK_OR  = 27,
    TOK_COM = 28,
    TOK_FUN = 29,
    TOK_RET = 30,
    TOK_TRU = 31,
    TOK_FAL = 32,
    TOK_DOT = 33,
    TOK_VAR = 34,
    TOK_CLS = 35,
    TOK_CON = 36,
    TOK_EXP = 37,
    TOK_IMP = 38
};

struct token {
    int code;     // token enum
    void* value;  // associated value ( for integer and string ) otherwise NULL
    int line;     // line from text
    int index;    // index from text
};

struct lexer {
    size_t current_index;

    const char* text;
    size_t text_size;

    size_t line;
};

int get_token(struct lexer* l, struct token* out_token);

#endif
