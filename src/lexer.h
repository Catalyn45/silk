#ifndef LEXER_H_
#define LEXER_H_

#include <stdint.h>

enum token {
    TOK_INT, // integer (20)
    TOK_STR, // string ("example")
    TOK_ADD, // add operator (+)
    TOK_MIN, // minux operator (-)
    TOK_MUL, // multiply operator (*)
    TOK_DIV, // division operator (/)
    TOK_EQL, // equal (=)
    TOK_LPR, // left parantesis (()
    TOK_RPR, // right parantesis ())
    TOK_LSQ, // left square parantesis ([)
    TOK_RSQ, // right square parantesis (])
    TOK_LBR, // left bracket ({)
    TOK_RBR, // right bracket (})
    TOK_EOF, // end of file
    TOK_IF,  // if
    TOK_ELS, // else
    TOK_WHL, // while
    TOK_IDN  // identifier
};

struct token_entry {
    int token_code;     // token enum
    void* token_value;  // associated value ( for integer and string ) otherwise NULL
};

int tokenize(const char* text, uint32_t text_size, struct token_entry** out_tokens, uint32_t* out_n_tokens);

#endif
