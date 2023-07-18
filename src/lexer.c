#include "lexer.h"
#include "utils.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 512

static int tokenize_int(uint32_t* current_index, const char* text, uint32_t text_size, struct token_entry* out_token) {
    int final_number = 0;

    while(*current_index < text_size && text[*current_index] >= '0' && text[*current_index] <= '9') {
        final_number = final_number * 10 + text[*current_index] - '0';
        ++(*current_index);
    }

    int* token_value = malloc(sizeof(*token_value));
    if (token_value == NULL) {
        ERROR("memory allocation failed");
        return 1;
    }

    *token_value = final_number;

    out_token->code = TOK_INT;
    out_token->value = token_value;

    return 0;
}

static int tokenize_string(uint32_t* current_index, const char* text, uint32_t text_size, struct token_entry* out_token) {
    char start_quote = text[(*current_index)++];

    char* value = malloc(CHUNK_SIZE);
    if (value == NULL) {
        ERROR("memory allocation failed");
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
        (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        character == '_'
    ) {
        return true;
    }

    return false;
}

static bool is_iden(char character) {
    return is_iden_first(character) || (character >= '0' && character <= '9');
}

int tokenize_identifier(uint32_t* current_index, const char* text, uint32_t text_size, struct token_entry* out_token) {
    char* value = malloc(CHUNK_SIZE);
    if (value == NULL) {
        ERROR("memory allocation failed");
        return 1;
    }

    uint32_t value_index = 0;

    while (*current_index < text_size && is_iden(text[*current_index])) {
        value[value_index++] = text[(*current_index)++];
    }

    value[value_index++] = '\0';

    if (strcmp(value, "if") == 0) {
        out_token->code = TOK_IF;
        goto keyword;
    }

    if (strcmp(value, "else") == 0) {
        out_token->code = TOK_ELS;
        goto keyword;
    }

    if (strcmp(value, "while") == 0) {
        out_token->code = TOK_WHL;
        goto keyword;
    }

    if (strcmp(value, "def") == 0) {
        out_token->code = TOK_FUN;
        goto keyword;
    }

    if (strcmp(value, "return") == 0) {
        out_token->code = TOK_RET;
        goto keyword;
    }

    out_token->code = TOK_IDN;
    out_token->value = value;

    return 0;

keyword:
    free(value);

    return 0;
}

#define SET_TOKEN(tok_code) \
    token->code = tok_code; \
    ++current_index;

int tokenize(const char* text, uint32_t text_size, struct token_entry** out_tokens, uint32_t* out_n_tokens) {
    struct token_entry* tokens = malloc(CHUNK_SIZE * sizeof(*tokens));
    if (tokens == NULL) {
        ERROR("memory allocation failed");
        return 1;
    }

    uint32_t n_tokens = 0;
    uint32_t current_index = 0;
    int line = 1;

    while (current_index < text_size) {
        char current_character = text[current_index];
        switch(current_character) {
            case '\n':
                line += 1;
            case ' ':
            case '\r':
            case '\t':
                ++current_index;
                continue;
        }

        uint32_t start_index = current_index;

        struct token_entry* token = &tokens[n_tokens++];
        *token = (struct token_entry) {0};

        switch (current_character) {
            case '+':
                SET_TOKEN(TOK_ADD);
                break;
            case '-':
                SET_TOKEN(TOK_MIN);
                break;
            case '/':
                SET_TOKEN(TOK_DIV);
                break;
            case '*':
                SET_TOKEN(TOK_MUL);
                break;
            case '=':
                if (current_index + 1 < text_size && text[current_index + 1] == '=') {
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
            case ',':
                SET_TOKEN(TOK_COM);
                break;
            case '<':
                if (current_index + 1 < text_size && text[current_index + 1] == '=') {
                    SET_TOKEN(TOK_LEQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_LES);
                }
                break;
            case '>':
                if (current_index + 1 < text_size && text[current_index + 1] == '=') {
                    SET_TOKEN(TOK_GRQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_GRE);
                }
                break;
            case '!':
                if (current_index + 1 < text_size && text[current_index + 1] == '=') {
                    SET_TOKEN(TOK_NEQ);
                    ++current_index;
                } else {
                    SET_TOKEN(TOK_NOT);
                }
                break;
            case '&':
                if (current_index + 1 < text_size && text[current_index + 1] == '&') {
                    SET_TOKEN(TOK_AND);
                    ++current_index;
                } else {
                    ERROR("invalid token: &%c", text[current_index + 1]);
                    return 1;
                }
                break;
            case '|':
                if (current_index + 1 < text_size && text[current_index + 1] == '|') {
                    SET_TOKEN(TOK_OR);
                    ++current_index;
                } else {
                    ERROR("invalid token: |%c", text[current_index + 1]);
                    return 1;
                }
                break;
            default:
                if (current_character >= '0' && current_character <= '9') {
                    int res = tokenize_int(&current_index, text, text_size, token);
                    if ( res != 0 ) {
                        // TODO: error
                        return 1;
                    }
                } else if (current_character == '\"' || current_character == '\'') {
                    int res = tokenize_string(&current_index, text, text_size, token);
                    if ( res != 0) {
                        // TODO: error
                        return 1;
                    }
                } else if (is_iden_first(current_character)) {
                    int res = tokenize_identifier(&current_index, text, text_size, token);
                    if ( res != 0 ) {
                        // TODO: error
                        return 1;
                    }
                } else {
                    ERROR("invalid character: %c\n", current_character);
                    goto free_tokens;
                }
                break;
        }

        token->line = line;
        token->index = start_index;
    }

    tokens[n_tokens++] = (struct token_entry) {
        .code = TOK_EOF
    };

    *out_tokens = tokens;
    *out_n_tokens = n_tokens;

    return 0;

free_tokens:
    free(tokens);

    return 1;
}
