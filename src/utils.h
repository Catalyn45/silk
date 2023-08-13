#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdbool.h>

struct node;

void disassembly(const uint8_t* bytes, uint32_t n_bytes, uint32_t start_address);
void dump_ast(struct node* root, int indent);
void print_program_error(const char* text, int32_t index);

extern const char* rev_tokens[];
extern const char* rev_objects[];

#define ERROR(message, ...) \
    printf("[ERROR] error: " message "\n" __VA_OPT__(,) __VA_ARGS__);

#define MEMORY_ERROR() \
    ERROR("failed to allocate memory");

#define EXPECT_TOKEN(token, expected_token) \
    if (token != expected_token) { \
        ERROR("expected token %s, got token %s", rev_tokens[expected_token], rev_tokens[token]); \
        return 1; \
    }

#define EXPECT_OBJECT(type, expected_type) \
{ \
    if (type != expected_type) {\
        ERROR("expected object type %s, got object type %s", rev_objects[expected_type], rev_objects[type]); \
        return 1; \
    } \
}

#define CHECK(result, message, ...) \
{ \
    if(result != 0) { \
        ERROR(message, __VA_ARGS__); \
        return 1;    \
    } \
}

#define CHECK_MEM(result) \
{ \
    if (result == NULL) { \
        MEMORY_ERROR(); \
        return 1; \
    } \
}

#define CHECK_NULL(result, message, ...) \
{ \
    if(result == NULL) { \
        ERROR(message, __VA_ARGS__); \
        return 1;    \
    } \
}

#endif
