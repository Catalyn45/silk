#include <gtest/gtest.h>
#include <cstdint>
#include <stdbool.h>

extern "C" {
    #include "../src/parser.h"
}

#define INIT() \
    struct lexer l = { \
        .text = input, \
        .text_size = sizeof(input) - 1, \
        .line = 1 \
    }; \
    struct parser p = { \
        .l = &l \
    }

#define EXPECT_NODE(node, expected_type) \
    EXPECT_EQ(node->type, expected_type)

#define TEST_PARSER(name) \
    TEST(test_parser, name)

TEST_PARSER(call) {
    const char input[] = "print(10)";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    EXPECT_EQ(statement->right, nullptr);

    struct node* exp_statement = statement->left;
    EXPECT_NODE(exp_statement, NODE_EXP_STATEMENT);

    struct node* call = exp_statement->left;
    EXPECT_NODE(call, NODE_CALL);

    struct node* var = call->left;
    EXPECT_NODE(var, NODE_VAR);

    EXPECT_STREQ((const char*)var->token.value, "print");

    struct node* argument = call->right;
    EXPECT_NODE(argument, NODE_ARGUMENT);

    struct node* number = argument->left;
    EXPECT_NODE(number, NODE_NUMBER);

    EXPECT_EQ(*((int32_t*)(number->token.value)), 10);
}

TEST_PARSER(declaration) {
    const char input[] = "var a";
    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* declaration = statement->left;
    EXPECT_NODE(declaration, NODE_DECLARATION);

    EXPECT_STREQ((const char*)declaration->token.value, "a");
}

TEST_PARSER(initialization) {
    const char input[] = "var a = 'hello'";
    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* declaration = statement->left;
    EXPECT_NODE(declaration, NODE_DECLARATION);

    EXPECT_STREQ((const char*)declaration->token.value, "a");

    struct node* init_value = declaration->left;
    EXPECT_NODE(init_value, NODE_STRING);

    EXPECT_STREQ((const char*)init_value->token.value, "hello");
}

TEST_PARSER(assignment) {
    const char input[] = "a = false";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* assignment = statement->left;
    EXPECT_NODE(assignment, NODE_ASSIGN);

    struct node* variable = assignment->left;
    EXPECT_NODE(variable, NODE_VAR);

    EXPECT_STREQ((const char*)variable->token.value, "a");

    struct node* init_value = assignment->right;
    EXPECT_NODE(init_value, NODE_BOOL);

    EXPECT_EQ(init_value->token.code, TOK_FAL);
}

TEST_PARSER(if) {
    const char input[] = "if (a > 10) {}";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* node_if = statement->left;
    EXPECT_NODE(node_if, NODE_IF);

    struct node* expression = node_if->left;
    EXPECT_NODE(expression, NODE_BINARY_OP);
    EXPECT_EQ(expression->token.code, TOK_GRE);

    struct node* variable = expression->left;
    EXPECT_NODE(variable, NODE_VAR);
    EXPECT_STREQ((const char*)variable->token.value, "a");

    struct node* value = expression->right;
    EXPECT_NODE(value, NODE_NUMBER);
    EXPECT_EQ(*((int32_t*)(value->token.value)), 10);

    struct node* decision = node_if->right;
    EXPECT_NODE(decision, NODE_DECISION);

    struct node* then = decision->left;
    EXPECT_NODE(then, NODE_BLOCK);

    struct node* els = decision->right;
    EXPECT_EQ(els, nullptr);
}

TEST_PARSER(if_else) {
    const char input[] = "if (a == 'hello') {} else {}";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* node_if = statement->left;
    EXPECT_NODE(node_if, NODE_IF);

    struct node* expression = node_if->left;
    EXPECT_NODE(expression, NODE_BINARY_OP);
    EXPECT_EQ(expression->token.code, TOK_DEQ);

    struct node* variable = expression->left;
    EXPECT_NODE(variable, NODE_VAR);
    EXPECT_STREQ((const char*)variable->token.value, "a");

    struct node* value = expression->right;
    EXPECT_NODE(value, NODE_STRING);
    EXPECT_STREQ((const char*)value->token.value, "hello");

    struct node* decision = node_if->right;
    EXPECT_NODE(decision, NODE_DECISION);

    struct node* then = decision->left;
    EXPECT_NODE(then, NODE_BLOCK);

    struct node* els = decision->right;
    EXPECT_NODE(els, NODE_BLOCK);
}

TEST_PARSER(while) {
    const char input[] = "while (!stop) {}";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* whl = statement->left;
    EXPECT_NODE(whl, NODE_WHILE);

    struct node* expression = whl->left;
    EXPECT_NODE(expression, NODE_NOT);

    struct node* variable = expression->left;
    EXPECT_NODE(variable, NODE_VAR);
    EXPECT_STREQ((const char*)variable->token.value, "stop");

    struct node* decision = whl->right;
    EXPECT_NODE(decision, NODE_DECISION);

    struct node* body = decision->left;
    EXPECT_NODE(body, NODE_BLOCK);
}

TEST_PARSER(function) {
    const char input[] = "def fun(a, b) {return a + b[-1]}";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* function = statement->left;
    EXPECT_NODE(function, NODE_FUNCTION);
    EXPECT_STREQ((const char*)function->token.value, "fun");

    struct node* parameter = function->left;
    EXPECT_NODE(parameter, NODE_PARAMETER);
    EXPECT_STREQ((const char*)parameter->token.value, "b");

    struct node* next_parameter = parameter->right;
    EXPECT_NODE(next_parameter, NODE_PARAMETER);
    EXPECT_STREQ((const char*)next_parameter->token.value, "a");

    struct node* body = function->right;
    EXPECT_NODE(body, NODE_BLOCK);

    struct node* body_statement = body->left;
    EXPECT_NODE(body_statement, NODE_STATEMENT);

    struct node* ret = body_statement->left;
    EXPECT_NODE(ret, NODE_RETURN);

    struct node* expression = ret->left;
    EXPECT_NODE(expression, NODE_BINARY_OP);
    EXPECT_EQ(expression->token.code, TOK_ADD);

    struct node* var1 = expression->left;
    EXPECT_NODE(var1, NODE_VAR);
    EXPECT_STREQ((const char*)var1->token.value, "a");

    struct node* var2 = expression->right;
    EXPECT_NODE(var2, NODE_INDEX);

    struct node* from = var2->left;
    EXPECT_NODE(from, NODE_VAR);
    EXPECT_STREQ((const char*)from->token.value, "b");

    struct node* index_expr = var2->right;
    EXPECT_NODE(index_expr, NODE_NUMBER);
    EXPECT_EQ(*((int32_t*)(index_expr->token.value)), -1);
}

TEST_PARSER(class) {
    const char input[] = "class Test { "
                            "var a "
                            "def get() { "
                                "return self.a "
                            "} "
                        "}";

    INIT();

    struct node* root;
    EXPECT_EQ(parse(&p, &root), 0);

    EXPECT_NODE(root, NODE_BLOCK);

    struct node* statement = root->left;
    EXPECT_NODE(statement, NODE_STATEMENT);

    struct node* cls = statement->left;
    EXPECT_NODE(cls, NODE_CLASS);
    EXPECT_STREQ((const char*)cls->token.value, "Test");

    struct node* member = cls->left;
    EXPECT_NODE(member, NODE_MEMBER);
    EXPECT_STREQ((const char*)member->token.value, "a");

    struct node* methods = cls->right;
    EXPECT_NODE(methods, NODE_METHODS);

    struct node* method = methods->left;
    EXPECT_NODE(method, NODE_METHOD);
    EXPECT_STREQ((const char*)method->token.value, "get");

    struct node* args = method->left;
    EXPECT_EQ(args, nullptr);

    struct node* method_body = method->right;
    EXPECT_NODE(method_body, NODE_BLOCK);

    struct node* return_stmt = method_body->left;
    EXPECT_NODE(return_stmt, NODE_STATEMENT);

    struct node* ret = return_stmt->left;
    EXPECT_NODE(ret, NODE_RETURN);

    struct node* ret_val = ret->left;
    EXPECT_NODE(ret_val, NODE_MEMBER_ACCESS);

    EXPECT_STREQ((const char*)ret_val->token.value, "a");

    struct node* from = ret_val->left;
    EXPECT_NODE(from, NODE_VAR);
    EXPECT_STREQ((const char*)from->token.value, "self");
}
