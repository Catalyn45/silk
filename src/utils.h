#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdbool.h> 

struct node;

void dump_ast(struct node* root, int indent, bool is_statement);
void print_program_error(const char* text, uint32_t index);

extern const char* rev_tokens[];

#define ERROR(message, ...) \
    printf("[ERROR] error: " message "\n" __VA_OPT__(,) __VA_ARGS__);

#define EXPECTED_TOKEN(token_code_expected, token_code_got) \
    ERROR("expected token %s, got token %s", rev_tokens[token_code_expected], rev_tokens[token_code_got]);

#endif
