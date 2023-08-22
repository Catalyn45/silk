#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

#define CHUNK_SIZE 512

#define in_range(character, start, stop) \
    ((character) >= (start) && (character) <= (stop))

static int tokenize_int(uint32_t* current_index, const char* text, uint32_t text_size, struct token* out_token) {
    int final_number = 0;
    int8_t sign = 1;

    if (text[*current_index] == '-') {
        sign = -1;
        ++(*current_index);
    }

    while(*current_index < text_size && in_range(text[*current_index], '0', '9')) {
        final_number = final_number * 10 + text[*current_index] - '0';
        ++(*current_index);
    }

    final_number = sign * final_number;

    int* token_value = malloc(sizeof(*token_value));
    if (token_value == NULL) {
        MEMORY_ERROR();
        return 1;
    }

    *token_value = final_number;

    out_token->code = TOK_INT;
    out_token->value = token_value;

    return 0;
}

// TODO: parse special characters like \n
static int tokenize_string(uint32_t* current_index, const char* text, uint32_t text_size, struct token* out_token) {
    char start_quote = text[(*current_index)++];

    char* value = malloc(CHUNK_SIZE);
    if (value == NULL) {
        MEMORY_ERROR();
        return 1;
    }

    uint32_t value_index = 0;

    while (*current_index < text_size && text[*current_index] != start_quote) {
        value[value_index++] = text[(*current_index)++];
    }

    if ( *current_index == text_size || text[*current_index] != start_quote) {
        ERROR("Expected closing quote on string");
        return 1;
    }

    ++(*current_index);
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
    };

    for (unsigned int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        if (strcmp(text, keywords[i].name) == 0) {
            *out_token_code = keywords[i].token;
            return true;
        }
    }

    return false;
}

static int tokenize_identifier(uint32_t* current_index, const char* text, uint32_t text_size, struct token* out_token) {
    char* value = malloc(CHUNK_SIZE);
    if (value == NULL) {
        MEMORY_ERROR();
        return 1;
    }

    uint32_t value_index = 0;

    while (*current_index < text_size && is_iden(text[*current_index])) {
        value[value_index++] = text[(*current_index)++];
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

#define SET_TOKEN(tok_code) \
    token->code = tok_code; \
    current_index++

#define next_character() \
    (text[current_index + 1])

#define is_end() \
    (current_index + 1 >= text_size)

void go_next_line(uint32_t* current_index, const char* text, uint32_t text_size) {
    while (*current_index < text_size && text[*current_index] != '\n') {
        ++(*current_index);
    }
}

int tokenize(const char* text, uint32_t text_size, struct token** out_tokens, uint32_t* out_n_tokens) {
    struct token* tokens = malloc(CHUNK_SIZE * sizeof(*tokens));
    if (tokens == NULL) {
        MEMORY_ERROR();
        return 1;
    }

    uint32_t n_tokens = 0;
    uint32_t current_index = 0;
    int line = 1;

    while (current_index < text_size) {
        char current_character = text[current_index];
        switch(current_character) {
            case '#':
                go_next_line(&current_index, text, text_size);
            case '\n':
                line += 1;
            case ' ':
            case '\r':
            case '\t':
                ++current_index;
                continue;
        }

        uint32_t start_index = current_index;

        struct token* token = &tokens[n_tokens++];
        *token = (struct token) {0};

        switch (current_character) {
            case '+':
                SET_TOKEN(TOK_ADD);
                break;
            case '-':
                if (!is_end() && in_range(next_character(), '0', '9')) {
                    CHECK(tokenize_int(&current_index, text, text_size, token), "failed to tokenize int");
                } else {
                    SET_TOKEN(TOK_MIN);
                }
                break;
            case '/':
                SET_TOKEN(TOK_DIV);
                break;
            case '*':
                SET_TOKEN(TOK_MUL);
                break;
            case '=':
                if (!is_end() && next_character() == '=') {
                    SET_TOKEN(TOK_DEQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_EQL);
                }
                break;
            case '(':
                SET_TOKEN(TOK_LPR);
                break;
            case ')':
                SET_TOKEN(TOK_RPR);
                break;
            case '{':
                SET_TOKEN(TOK_LBR);
                break;
            case '}':
                SET_TOKEN(TOK_RBR);
                break;
            case '[':
                SET_TOKEN(TOK_LSQ);
                break;
            case ']':
                SET_TOKEN(TOK_RSQ);
                break;
            case '.':
                SET_TOKEN(TOK_DOT);
                break;
            case ',':
                SET_TOKEN(TOK_COM);
                break;
            case '<':
                if (!is_end() && next_character() == '=') {
                    SET_TOKEN(TOK_LEQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_LES);
                }
                break;
            case '>':
                if (!is_end() && next_character() == '=') {
                    SET_TOKEN(TOK_GRQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_GRE);
                }
                break;
            case '!':
                if (!is_end() && next_character() == '=') {
                    SET_TOKEN(TOK_NEQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_NOT);
                }
                break;
            case '&':
                if (!is_end() && next_character() == '&') {
                    SET_TOKEN(TOK_AND);
                    ++current_index;
                } else {
                    ERROR("invalid token: &%c", next_character());
                    return 1;
                }
                break;
            case '|':
                if (!is_end() && next_character() == '|') {
                    SET_TOKEN(TOK_OR);
                    ++current_index;
                } else {
                    ERROR("invalid token: |%c", next_character());
                    return 1;
                }
                break;
            default:
                if (in_range(current_character, '0', '9')) {
                    CHECK(tokenize_int(&current_index, text, text_size, token), "failed to tokenize int");

                } else if (current_character == '\"' || current_character == '\'') {
                    CHECK(tokenize_string(&current_index, text, text_size, token), "failed to tokenize string");

                } else if (is_iden_first(current_character)) {
                    CHECK(tokenize_identifier(&current_index, text, text_size, token), "failed to tokenize identifier");

                } else {
                    ERROR("invalid character: %c\n", current_character);
                    goto free_tokens;
                }
                break;
        }

        token->line = line;
        token->index = start_index;
    }

    tokens[n_tokens++] = (struct token) {
        .code = TOK_EOF
    };

    *out_tokens = tokens;
    *out_n_tokens = n_tokens;

    return 0;

free_tokens:
    free(tokens);

    return 1;
}
