#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdbool.h> 

struct node;

void disassembly(const uint8_t* bytes, uint32_t n_bytes);
void dump_ast(struct node* root, int indent);
void print_program_error(const char* text, int32_t index);
extern const char* rev_tokens[];

#define ERROR(message, ...) \
    printf("[ERROR] error: " message "\n" __VA_OPT__(,) __VA_ARGS__);

#define MEMORY_ERROR() \
    ERROR("failed to allocate memory");

#define EXPECT_TOKEN(token, expected_token) \
    if (token != expected_token) { \
        ERROR("expected token %s, got token %s", rev_tokens[expected_token], rev_tokens[token]); \
        return 1; \
    }

#define CHECK(result, message, ...) \
{ \
    if(result != 0) { \
        ERROR(message, __VA_ARGS__); \
        return 1;    \
    } \
}

#endif
