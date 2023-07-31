#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "utils.h"

#define get_token() \
    (&parser->tokens[parser->current_index])

#define next_token() \
    (&parser->tokens[parser->current_index + 1])

#define advance() \
    (++parser->current_index)

#define CHECK_NODE(node) \
{\
    if (!node) {\
        MEMORY_ERROR();\
        return 1;\
    }\
}

static int parse_expression(struct parser* parser, struct node** root);

static int parse_primary(struct parser* parser, struct node** root) {
    const struct token* current_token = get_token();

    if (current_token->code == TOK_LPR) {
        advance();

        CHECK(parse_expression(parser, root), "failed to parse expression");

        current_token = get_token();

        EXPECT_TOKEN(current_token->code, TOK_RPR);
        advance();

        return 0;
    }

    if (current_token->code == TOK_INT) {
        advance();

        struct node* node_num = node_new(NODE_NUMBER, current_token, NULL, NULL, parser->current_scope);
        CHECK_NODE(node_num);

        *root = node_num;
        return 0;
    }

    if (current_token->code == TOK_TRU || current_token->code == TOK_FAL) {
        advance();

        struct node* node_bool = node_new(NODE_BOOL, current_token, NULL, NULL, parser->current_scope);
        CHECK_NODE(node_bool);

        *root = node_bool;
        return 0;
    }

    if (current_token->code == TOK_STR) {
        advance();

        struct node* node_num = node_new(NODE_STRING, current_token, NULL, NULL, parser->current_scope);
        CHECK_NODE(node_num);

        *root = node_num;
        return 0;
    }

    if (current_token->code == TOK_IDN) {
        advance();
        struct node* node_var = node_new(NODE_VAR, current_token, NULL, NULL, parser->current_scope);
        CHECK_NODE(node_var);
        node_var->flags |= (LVALUE | CALLABLE);

        *root = node_var;
        return 0;
    }

    ERROR("unexpected token %s", rev_tokens[current_token->code]);
    return 1;
}

static int parse_argument_list(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_LPR);
    advance();

    struct node* arguments = NULL;

    current_token = get_token();

    while (current_token->code != TOK_RPR) {
        struct node* argument;
        CHECK(parse_expression(parser, &argument), "failed to parse argument");

        current_token = get_token();
        if (current_token->code != TOK_RPR) {
            EXPECT_TOKEN(current_token->code, TOK_COM);
            advance();
            current_token = get_token();
        }

        arguments = node_new(NODE_ARGUMENT, NULL, argument, arguments, parser->current_scope);
        CHECK_NODE(arguments);
    }

    advance();

    *root = arguments;
    return 0;
}

static int parse_postfix(struct parser* parser, struct node** root) {
    struct node* left = NULL;
    CHECK(parse_primary(parser, &left), "failed to parse primary");

    const struct token* current_token = get_token();

    while (true) {
        if (current_token->code == TOK_LPR && (left->flags & CALLABLE)) {
            struct node* function_parameters = NULL;
            CHECK(parse_argument_list(parser, &function_parameters), "failed to parse argument list");
            struct node* function_node = node_new(NODE_FUNCTION_CALL, NULL, left, function_parameters, parser->current_scope);
            function_node->flags |= CALLABLE;

            left = function_node;
        } else if (current_token->code == TOK_DOT) {
            advance();

            current_token = get_token();

            EXPECT_TOKEN(current_token->code, TOK_IDN);

            struct node* identifier = node_new(NODE_VAR, current_token, NULL, NULL, parser->current_scope);
            CHECK_NODE(identifier);

            struct node* member_access = node_new(NODE_MEMBER, NULL, left, identifier, parser->current_scope);
            CHECK_NODE(member_access);

            member_access->flags |= (LVALUE | CALLABLE);

            left = member_access;
        } else if (current_token->code == TOK_LSQ) {
            advance();

            struct node* expression;
            CHECK(parse_expression(parser, &expression), "failed to parse expression");

            current_token = get_token();

            EXPECT_TOKEN(current_token->code, TOK_RSQ);
            advance();

            struct node* index_access = node_new(NODE_INDEX, NULL, left, expression, parser->current_scope);
            CHECK_NODE(index_access);

            index_access->flags |= (LVALUE | CALLABLE);

            left = index_access;
        } else {
            break;
        }

        current_token = get_token();
    }
    *root = left;
    return 0;
}

static int parse_unary(struct parser* parser, struct node** root) {
    const struct token* current_token = get_token();

    if (current_token->code == TOK_NOT) {
        advance();

        struct node* not_unary;
        CHECK(parse_unary(parser, &not_unary), "failed to parse unary");

        struct node* not_node = node_new(NODE_NOT, NULL, not_unary, NULL, parser->current_scope);
        CHECK_NODE(not_node);

        *root = not_node;

        return 0;
    }

    CHECK(parse_postfix(parser, root), "failed to parse postfix");
    return 0;
}

static int parse_multiplicative(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_unary(parser, &left), "failed to parse unary");

    const struct token* current_token = get_token();
    while (
        current_token->code == TOK_MUL  ||
        current_token->code == TOK_DIV
    ) {
        advance();

        struct node* right;
        CHECK(parse_unary(parser, &right), "failed to parse unary");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_additive(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_multiplicative(parser, &left), "failed to parse multiplicative");

    const struct token* current_token = get_token();
    while (
        current_token->code == TOK_ADD  ||
        current_token->code == TOK_MIN
    ) {
        advance();

        struct node* right;
        CHECK(parse_multiplicative(parser, &right), "failed to parse multiplicative");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_relational(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_additive(parser, &left), "failed to parse additive");

    const struct token* current_token = get_token();
    while (
        current_token->code == TOK_LES  ||
        current_token->code == TOK_LEQ  ||
        current_token->code == TOK_GRE  ||
        current_token->code == TOK_GRQ
    ) {
        advance();

        struct node* right;
        CHECK(parse_additive(parser, &right), "failed to parse additive");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_equality(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_relational(parser, &left), "failed to parse relational");

    const struct token* current_token = get_token();
    while (current_token->code == TOK_DEQ || current_token->code == TOK_NEQ) {
        advance();

        struct node* right;
        CHECK(parse_relational(parser, &right), "failed to parse relational");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_and(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_equality(parser, &left), "failed to parse equality");

    const struct token* current_token = get_token();
    while (current_token->code == TOK_AND) {
        advance();

        struct node* right;
        CHECK(parse_equality(parser, &right), "failed to parse equality");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_or(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_and(parser, &left), "failed to parse and");

    const struct token* current_token = get_token();
    while (current_token->code == TOK_OR) {
        advance();

        struct node* right;
        CHECK(parse_and(parser, &right), "failed to parse and");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_expression(struct parser* parser, struct node** root) {
    CHECK(parse_or(parser, root), "failed to parse or");
    return 0;
}

static int parse_expression_statement(struct parser* parser, struct node** root) {
    struct node* expression;
    CHECK(parse_expression(parser, &expression), "Failed to pare expression");

    const struct token* current_token = get_token();
    if (current_token->code == TOK_EQL &&  (expression->flags & LVALUE)) {
        advance();

        struct node* assignment_value;
        CHECK(parse_expression(parser, &assignment_value), "failed to parse expression");

        struct node* assignment_node = node_new(NODE_ASSIGN, NULL, expression, assignment_value, parser->current_scope);
        CHECK_NODE(assignment_node);

        *root = assignment_node;
        return 0;
    }

    struct node* expression_statement_node = node_new(NODE_EXP_STATEMENT, NULL, expression, NULL, parser->current_scope);
    CHECK_NODE(expression_statement_node);

    *root = expression_statement_node;
    return 0;
}

static int parse_block(struct parser* parser, struct node** root, bool brackets);

static int parse_if(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_IF)
    advance();

    struct node* if_expression;
    CHECK(parse_expression(parser, &if_expression), "failed to parse expression");

    struct node* true_side;
    CHECK(parse_block(parser, &true_side, true), "failed to parse block");

    struct node* false_side = NULL;
    current_token = get_token();
    if (current_token->code == TOK_ELS) {
        advance();

        CHECK(parse_block(parser, &false_side, true), "failed to parse block");
    }

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, false_side, parser->current_scope);
    CHECK_NODE(decision);

    struct node* if_node = node_new(NODE_IF, NULL, if_expression, decision, parser->current_scope);
    CHECK_NODE(if_node);

    *root = if_node;
    return 0;
}

static int parse_while(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_WHL);
    advance();

    struct node* while_expression;
    CHECK(parse_expression(parser, &while_expression), "failed to parse expression");

    struct node* true_side;
    CHECK(parse_block(parser, &true_side, true), "failed to parse block");

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, NULL, parser->current_scope);
    CHECK_NODE(decision);

    struct node* while_node = node_new(NODE_WHILE, NULL, while_expression, decision, parser->current_scope);
    CHECK_NODE(while_node);

    *root = while_node;

    return 0;
}


static int parse_parameter_list(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_LPR);
    advance();

    struct node* parameters = NULL;

    current_token = get_token();

    while (current_token->code != TOK_RPR) {
        EXPECT_TOKEN(current_token->code, TOK_IDN);
        const struct token* parameter_name = current_token;
        advance();

        current_token = get_token();
        if (current_token->code != TOK_RPR) {
            EXPECT_TOKEN(current_token->code, TOK_COM);
            advance();
            current_token = get_token();
        }

        parameters = node_new(NODE_PARAMETER, parameter_name, NULL, parameters, parser->current_scope);
        CHECK_NODE(parameters);
    }

    advance();

    *root = parameters;
    return 0;
}

static int parse_function(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_FUN);
    advance();

    const struct token* function_name = get_token();
    EXPECT_TOKEN(function_name->code, TOK_IDN);
    advance();

    struct node* arguments;
    CHECK(parse_parameter_list(parser, &arguments), "failed to parse parameter list");

    struct node* body;
    CHECK(parse_block(parser, &body, true), "failed to parse block");

    struct node* function = node_new(NODE_FUNCTION, function_name, arguments, body, parser->current_scope);
    CHECK_NODE(function);

    *root = function;
    return 0;
}

static int parse_return(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_RET);
    advance();

    struct node* expression;
    CHECK(parse_expression(parser, &expression), "failed to parse expression");

    struct node* return_node = node_new(NODE_RETURN, NULL, expression, NULL, parser->current_scope);
    CHECK_NODE(return_node);

    *root = return_node;

    return 0;
}

static int parse_declaration(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_VAR);
    advance();

    current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_IDN);

    struct node* var = node_new(NODE_VAR, current_token, NULL, NULL, parser->current_scope);
    CHECK_NODE(var);

    current_token = get_token();
    struct node* init_value = NULL;
    if (current_token->code == TOK_EQL) {
        CHECK(parse_expression(parser, &init_value), "failed to parse expression");
    }

    struct node* declaration = node_new(NODE_DECLARATION, NULL, var, init_value, parser->current_scope);
    CHECK_NODE(declaration);

    *root = declaration;
    return 0;
}

static int parse_class(struct parser* parser, struct node** root ) {
    const struct token* current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_CLS);
    advance();

    current_token = get_token();

    EXPECT_TOKEN(current_token->code, TOK_IDN);
    const struct token* class_name = current_token;
    advance();

    current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_LBR);
    advance();

    current_token = get_token();

    struct node* members = NULL;
    while (current_token->code == TOK_VAR) {
        struct node* member;
        CHECK(parse_declaration(parser, &member), "failed to parse member");
        members = node_new(NODE_MEMBER, NULL, member, members, parser->current_scope);
        CHECK_NODE(members);
    }

    struct node* methods = NULL;
    while (current_token->code == TOK_FUN) {
        struct node* method;
        CHECK(parse_function(parser, &method), "failed to parse method");
        methods = node_new(NODE_METHOD, NULL, method, methods, parser->current_scope);
        CHECK_NODE(methods);
    }

    EXPECT_TOKEN(current_token->code, TOK_RBR);
    advance();

    struct node* class_node = node_new(NODE_CLASS, class_name, members, methods, parser->current_scope);
    CHECK_NODE(class_node);

    *root = class_node;
    return 0;
}

static int parse_statement(struct parser* parser, struct node** root ) {
    int current_token_code = get_token()->code;

    if (current_token_code == TOK_IF) {
        CHECK(parse_if(parser, root), "failed to parse if statement");
        return 0;
    }

    if (current_token_code == TOK_WHL) {
        CHECK(parse_while(parser, root), "failed to parse while statement");
        return 0;
    }

    if (current_token_code == TOK_FUN) {
        CHECK(parse_function(parser, root), "failed to parse function");
        return 0;
    }

    if (current_token_code == TOK_VAR) {
        CHECK(parse_declaration(parser, root), "failed to parse declaration");
        return 0;
    }

    if (current_token_code == TOK_CLS) {
        CHECK(parse_class(parser, root), "failed to parse class");
        return 0;
    }

    if (current_token_code == TOK_RET) {
        CHECK(parse_return(parser, root), "failed to parse return");
        return 0;
    }

    CHECK(parse_expression_statement(parser, root), "failed to parse expression statement");
    return 0;
}

static int parse_block(struct parser* parser, struct node** root, bool brackets) {
    const struct token* current_token = get_token();

    if (brackets) {
        EXPECT_TOKEN(current_token->code, TOK_LBR);
        advance();

        ++parser->current_scope;
    }

    struct node* statements = NULL;;
    while (current_token->code != TOK_RBR && current_token->code != TOK_EOF) {
        struct node* statement;
        CHECK(parse_statement(parser, &statement), "failed to parse statement");

        statements = node_new(NODE_STATEMENT, NULL, statement, statements, parser->current_scope);
        CHECK_NODE(statements);

        current_token = get_token();
    }

    if (brackets) {
        EXPECT_TOKEN(current_token->code, TOK_RBR);
        advance();

        --parser->current_scope;
    } else {
        EXPECT_TOKEN(current_token->code, TOK_EOF);
    }

    *root = statements;
    return 0;
}

int parse(struct parser* parser, struct node** root) {
    uint32_t current_index = 0;
    int res = parse_block(parser, root, false);
    if (res != 0) {
        const struct token* token = &parser->tokens[current_index];
        ERROR("at line %d", token->line);
        print_program_error(parser->text, token->index);
    }

    return res;
}
