#include <gtest/gtest.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
    #include "../src/sylk.h"
}

#define TEST_RUN(name) \
    TEST(test_run, name)

#define RUN(file_name, expected_output) \
{ \
    struct sylk* s = sylk_new(NULL, NULL); \
    sylk_load_prelude(s); \
\
    FILE* output = fopen("./output.txt", "w+"); \
    EXPECT_NE(output, nullptr); \
\
    int old_stdout = dup(STDOUT_FILENO); \
\
    close(STDOUT_FILENO); \
    dup2(fileno(output), STDOUT_FILENO); \
\
    int res = sylk_run_file(s, "./test/sources/" file_name); \
    sylk_free(s); \
\
    close(STDOUT_FILENO); \
    dup2(old_stdout, STDOUT_FILENO); \
    close(old_stdout); \
\
    EXPECT_EQ(res, 0); \
    char content[200]; \
    fseek(output, 0, SEEK_SET); \
    size_t n_read = fread(content, sizeof(char), sizeof(content), output); \
    content[n_read] = '\0'; \
\
    EXPECT_STREQ(content, expected_output "\n");\
\
    fclose(output); \
}

TEST_RUN(fibonacci) {
    RUN("fibonacci_recursive.slk", "55");
    RUN("fibonacci.slk", "89");
}

TEST_RUN(two_power) {
    RUN("2_power.slk", "1024");
    RUN("2_power_recursive.slk", "2048");
}

TEST_RUN(classes) {
    RUN("classes.slk", "3");
    RUN("member_access.slk", "33");
    RUN("index_access.slk", "155");
    RUN("list.slk", "60");
}

TEST_RUN(conversions) {
    RUN("strings.slk", "hello10")
    RUN("numbers.slk", "77")
}

