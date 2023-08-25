#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

#define CHUNK_SIZE 512

#define in_range(character, start, stop) \
    ((character) >= (start) && (character) <= (stop))

#define is_end() \
    (l->current_index >= l->text_size)

#define is_last() \
    (l->current_index + 1 >= l->text_size)

#define current_char() \
    (l->text[l->current_index])

#define advance() \
    (++l->current_index);

static int tokenize_int(struct lexer* l, struct token* out_token) {
    int final_number = 0;
    int8_t sign = 1;

    if (current_char() == '-') {
        sign = -1;
        advance();
    }


    while(!is_end() && in_range(current_char(), '0', '9')) {
        final_number = final_number * 10 + current_char() - '0';
        advance();
    }

    final_number = sign * final_number;

    int* token_value = malloc(sizeof(*token_value));
    CHECK_MEM(token_value);

    *token_value = final_number;

    out_token->code = TOK_INT;
    out_token->value = token_value;

    return 0;
}

// TODO: parse special characters like \n
static int tokenize_string(struct lexer* l, struct token* out_token) {
    char start_quote = current_char();
    advance();

    char* value = malloc(CHUNK_SIZE);
    CHECK_MEM(value);

    uint32_t value_index = 0;

    while (!is_end() && current_char() != start_quote) {
        value[value_index++] = l->text[l->current_index++];
    }

    if (is_end() || current_char() != start_quote) {
        ERROR("Expected closing quote on string");
        return 1;
    }

    advance();
    value[value_index++] = '\0';

    out_token->code = TOK_STR;
    out_token->value = value;

    return 0;
}

static bool is_iden_first(char character) {
    if (
        (in_range(character, 'a', 'z')) ||
        (in_range(character, 'A', 'Z')) ||
        character == '_'
    ) {
        return true;
    }

    return false;
}

static bool is_iden(char character) {
    return is_iden_first(character) || in_range(character, '0', '9');
}

static bool check_keyword(const char* text, int* out_token_code) {
    static const struct {
        const char* name;
        int token;
    } keywords[] = {
        {.name = "if",     .token = TOK_IF},
        {.name = "else",   .token = TOK_ELS},
        {.name = "while",  .token = TOK_WHL},
        {.name = "def",    .token = TOK_FUN},
        {.name = "return", .token = TOK_RET},
        {.name = "true",   .token = TOK_TRU},
        {.name = "false",  .token = TOK_FAL},
        {.name = "var",    .token = TOK_VAR},
        {.name = "class",  .token = TOK_CLS},
        {.name = "const",  .token = TOK_CON},
        {.name = "export", .token = TOK_EXP},
        {.name = "import", .token = TOK_IMP},
    };

    for (unsigned int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        if (strcmp(text, keywords[i].name) == 0) {
            *out_token_code = keywords[i].token;
            return true;
        }
    }

    return false;
}

static int tokenize_identifier(struct lexer* l, struct token* out_token) {
    char* value = malloc(CHUNK_SIZE);
    CHECK_MEM(value);

    uint32_t value_index = 0;

    while (!is_end() && is_iden(current_char())) {
        value[value_index++] = current_char();
        advance();
    }

    value[value_index++] = '\0';

    int token_code;
    if (check_keyword(value, &token_code)) {
        free(value);
        out_token->code = token_code;
        return 0;
    }

    out_token->code = TOK_IDN;
    out_token->value = value;

    return 0;
}

#define set_token(tok_code) \
    out_token->code = tok_code; \
    advance();

#define next_character() \
    (l->text[l->current_index + 1])

void go_next_line(struct lexer* l) {
    while (!is_end() && current_char() != '\n') {
        advance();
    }
}

int get_token(struct lexer* l, struct token* out_token) {
    while (!is_end()) {
        switch(current_char()) {
            case '#':
                go_next_line(l);
            case '\n':
                l->line += 1;
            case ' ':
            case '\r':
            case '\t':
                advance();
                continue;
        }
        break;
    }

    if (is_end()) {
        out_token->code = TOK_EOF;
        return 0;
    }

    uint32_t start_index = l->current_index;
    char current_character = current_char();

    switch (current_character) {
        case '+':
            set_token(TOK_ADD);
            break;
        case '-':
            if (!is_last() && in_range(next_character(), '0', '9')) {
                CHECK(tokenize_int(l, out_token), "failed to tokenize int");
            } else {
                set_token(TOK_MIN);
            }
            break;
        case '/':
            set_token(TOK_DIV);
            break;
        case '*':
            set_token(TOK_MUL);
            break;
        case '=':
            if (!is_last() && next_character() == '=') {
                set_token(TOK_DEQ);
                advance();
            } else {
                set_token(TOK_EQL);
            }
            break;
        case '(':
            set_token(TOK_LPR);
            break;
        case ')':
            set_token(TOK_RPR);
            break;
        case '{':
            set_token(TOK_LBR);
            break;
        case '}':
            set_token(TOK_RBR);
            break;
        case '[':
            set_token(TOK_LSQ);
            break;
        case ']':
            set_token(TOK_RSQ);
            break;
        case '.':
            set_token(TOK_DOT);
            break;
        case ',':
            set_token(TOK_COM);
            break;
        case '<':
            if (!is_last() && next_character() == '=') {
                set_token(TOK_LEQ);
                advance();
            } else {
                set_token(TOK_LES);
            }
            break;
        case '>':
            if (!is_last() && next_character() == '=') {
                set_token(TOK_GRQ);
                advance();
            } else {
                set_token(TOK_GRE);
            }
            break;
        case '!':
            if (!is_last() && next_character() == '=') {
                set_token(TOK_NEQ);
                advance();
            } else {
                set_token(TOK_NOT);
            }
            break;
        case '&':
            if (!is_last() && next_character() == '&') {
                set_token(TOK_AND);
                advance();
            } else {
                ERROR("invalid token after &");
                return 1;
            }
            break;
        case '|':
            if (!is_last() && next_character() == '|') {
                set_token(TOK_OR);
                advance();
            } else {
                ERROR("invalid token after |");
                return 1;
            }
            break;
        default:
            if (in_range(current_character, '0', '9')) {
                CHECK(tokenize_int(l, out_token), "failed to tokenize int");

            } else if (current_character == '\"' || current_character == '\'') {
                CHECK(tokenize_string(l, out_token), "failed to tokenize string");

            } else if (is_iden_first(current_character)) {
                CHECK(tokenize_identifier(l, out_token), "failed to tokenize identifier");

            } else {
                ERROR("invalid character: %c\n", current_character);
                return 1;
            }

            break;
    }

    out_token->index = start_index;
    return 0;
}
