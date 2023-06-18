#ifndef LEXER_H_
#define LEXER_H_

#include <stdint.h>

enum token {
    TOK_INT = 0, // integer (20)
    TOK_STR = 1, // string ("example")
    TOK_ADD = 2, // add operator (+)
    TOK_MIN = 3, // minux operator (-)
    TOK_MUL = 4, // multiply operator (*)
    TOK_DIV = 5, // division operator (/)
    TOK_EQL = 6, // equal (=)
    TOK_LPR = 7, // left parantesis (()
    TOK_RPR = 8, // right parantesis ())
    TOK_LSQ = 9, // left square parantesis ([)
    TOK_RSQ = 10, // right square parantesis (])
    TOK_LBR = 11, // left bracket ({)
    TOK_RBR = 12, // right bracket (})
    TOK_EOF = 13, // end of file
    TOK_IF =  14,  // if
    TOK_ELS = 15, // else
    TOK_WHL = 16, // while
    TOK_IDN = 17, // identifier
    TOK_LES = 18,
    TOK_LEQ = 19,
    TOK_GRE = 20,
    TOK_GRQ = 21,
    TOK_DEQ = 22,
    TOK_NEQ = 23,
    TOK_NOT = 24,
    TOK_AND = 25,
    TOK_OR =  26
};

struct token_entry {
    int token_code;     // token enum
    void* token_value;  // associated value ( for integer and string ) otherwise NULL
    int line;           // line from text
    int column;         // column from text
    int index;          // index from text
};

int tokenize(const char* text, uint32_t text_size, struct token_entry** out_tokens, uint32_t* out_n_tokens);

#endif
