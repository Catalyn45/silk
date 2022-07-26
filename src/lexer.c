#include "lexer.h"
#include <stdio.h>

#include <stdlib.h>

#define CHUNK_SIZE 40

static int tokenize_int(uint32_t* current_index, const char* text, uint32_t text_size, struct token_entry** tokens, uint32_t* n_tokens) {
    int final_number = 0;

    while(*current_index < text_size && text[*current_index] >= '0' && text[*current_index] <= '9') {
        final_number = final_number * 10 + text[*current_index] - '0';
        ++(*current_index);
    }

    int* token_value = malloc(sizeof(*token_value));
    if (token_value == NULL) {
        // TODO: error
        return 1;
    }

    *token_value = final_number;

    (*tokens)[(*n_tokens)++] = (struct token_entry) {
        .token_code = TOK_INT,
        .token_value = token_value
    };

    return 0;
}

static int tokenize_string(uint32_t* current_index, const char* text, uint32_t text_size, struct token_entry** tokens, uint32_t* n_tokens) {
    char start_quote = text[(*current_index)++];

    char* value = malloc(CHUNK_SIZE);
    if (value == NULL) {
        // TODO: error
        return 1;
    }

    uint32_t allocated = CHUNK_SIZE;
    uint32_t value_index = 0;

    while ( *current_index < text_size && text[*current_index] != start_quote ) {
        if (value_index >= allocated) {
            char* new_mem = realloc(value, allocated + CHUNK_SIZE);
            if (new_mem == NULL) {
                // TODO: error
                return 1;
            }

            value = new_mem;
            allocated += CHUNK_SIZE;
        }

        value[value_index++] = text[(*current_index)++];
    }

    if ( *current_index == text_size || text[*current_index] != start_quote) {
        // TODO: error
        return 1;
    } else {
        ++(*current_index);
    }

    (*tokens)[(*n_tokens)++] = (struct token_entry) {
        .token_code = TOK_STR,
        .token_value = value
    };

    return 0;
}

int tokenize(const char* text, uint32_t text_size, struct token_entry** out_tokens, uint32_t* out_n_tokens) {
    uint32_t current_index = 0;

    struct token_entry* tokens = malloc(CHUNK_SIZE * sizeof(*tokens));
    if (tokens == NULL) {
        // TODO: error
        return 1;
    }

    uint32_t allocated = CHUNK_SIZE;
    uint32_t n_tokens = 0;

#define ADD_TOKEN(code)       \
    tokens[n_tokens++] = (struct token_entry) {           \
        .token_code = code    \
    }; \
    ++current_index;

    while ( current_index < text_size ) {
        if (n_tokens >= allocated) {
            struct token_entry* new_mem = realloc(tokens, allocated + CHUNK_SIZE);
            if (new_mem == NULL) {
                // TODO: error
                return 1;
            }

            tokens = new_mem;
            allocated += CHUNK_SIZE;
        }

        switch(text[current_index]) {
            case '+':
                ADD_TOKEN(TOK_ADD);
                break;
            case '-':
                ADD_TOKEN(TOK_MIN);
                break;
            case '/':
                ADD_TOKEN(TOK_DIV);
                break;
            case '*':
                ADD_TOKEN(TOK_MUL);
                break;
            case '=':
                ADD_TOKEN(TOK_EQL);
                break;
            case '(':
                ADD_TOKEN(TOK_LPR);
                break;
            case ')':
                ADD_TOKEN(TOK_RPR);
                break;
            default:
                if (text[current_index] >= '0' && text[current_index] <= '9') {
                    int res = tokenize_int(&current_index, text, text_size, &tokens, &n_tokens);
                    if ( res != 0 ) {
                        // TODO: error
                        return 1;
                    }
                } else if (text[current_index] == '\"' || text[current_index] == '\'') {
                    int res = tokenize_string(&current_index, text, text_size, &tokens, &n_tokens);
                    if ( res != 0) {
                        // TODO: error
                        return 1;
                    }
                } else if ( text[current_index] == ' ' || text[current_index] == '\t' ) {
                    ++current_index;
                }
        }
    }

    tokens[n_tokens++] = (struct token_entry) {
        .token_code = TOK_EOF
    };

    *out_tokens = tokens;
    *out_n_tokens = n_tokens;

    return 0;
}
