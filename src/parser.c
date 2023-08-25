#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "ast.h"
#include "utils.h"
#include "lexer.h"


#define advance() \
    get_token(parser->l, &parser->current_token)

#define get_current_token() \
    (&parser->current_token)

#define CHECK_NODE(node) \
{\
    if (!node) {\
        MEMORY_ERROR();\
        return 1;\
    }\
}

// unused for now
struct context {
};

static int parse_expression(struct parser* parser, struct node** root, struct context* ctx);

static int parse_literal(struct parser* parser, struct node** root, struct context* ctx) {
    (void)ctx;
    const struct token* current_token = get_current_token();

    if (current_token->code == TOK_INT) {
        struct node* node_num = node_new(NODE_NUMBER, current_token, NULL, NULL);
        CHECK_NODE(node_num);

        advance();
        *root = node_num;
        return 0;
    }

    if (current_token->code == TOK_TRU || current_token->code == TOK_FAL) {
        struct node* node_bool = node_new(NODE_BOOL, current_token, NULL, NULL);
        CHECK_NODE(node_bool);

        advance();
        *root = node_bool;
        return 0;
    }

    if (current_token->code == TOK_STR) {
        struct node* node_num = node_new(NODE_STRING, current_token, NULL, NULL);
        CHECK_NODE(node_num);

        advance();
        *root = node_num;
        return 0;
    }

    ERROR("unexpected token %s", rev_tokens[current_token->code]);
    return 1;
}

static int parse_primary(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    if (current_token->code == TOK_LPR) {
        advance();

        CHECK(parse_expression(parser, root, ctx), "failed to parse expression");

        current_token = get_current_token();

        EXPECT_TOKEN(current_token->code, TOK_RPR);
        advance();

        return 0;
    }


    if (current_token->code == TOK_IDN) {
        struct node* node_var = node_new(NODE_VAR, current_token, NULL, NULL);
        CHECK_NODE(node_var);

        node_var->flags |= (LVALUE | CALLABLE);

        advance();
        *root = node_var;
        return 0;
    }

    CHECK(parse_literal(parser, root, ctx), "failed to parse literal");
    return 0;
}

static int parse_argument_list(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_LPR);
    advance();

    struct node* arguments = NULL;

    current_token = get_current_token();

    while (current_token->code != TOK_RPR) {
        struct node* argument;
        CHECK(parse_expression(parser, &argument, ctx), "failed to parse argument");

        current_token = get_current_token();
        if (current_token->code != TOK_RPR) {
            EXPECT_TOKEN(current_token->code, TOK_COM);
            advance();
            current_token = get_current_token();
        }

        arguments = node_new(NODE_ARGUMENT, NULL, argument, arguments);
        CHECK_NODE(arguments);
    }

    advance();

    *root = arguments;
    return 0;
}

static int parse_postfix(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left = NULL;
    CHECK(parse_primary(parser, &left, ctx), "failed to parse primary");

    const struct token* current_token = get_current_token();

    while (true) {
        if (current_token->code == TOK_LPR && (left->flags & CALLABLE)) {
            struct node* function_parameters = NULL;
            CHECK(parse_argument_list(parser, &function_parameters, ctx), "failed to parse argument list");
            struct node* function_node = node_new(NODE_CALL, NULL, left, function_parameters);
            function_node->flags |= CALLABLE;

            left = function_node;
        } else if (current_token->code == TOK_DOT) {
            advance();

            current_token = get_current_token();

            EXPECT_TOKEN(current_token->code, TOK_IDN);
            struct node* member_access = node_new(NODE_MEMBER_ACCESS, current_token, left, NULL);
            advance();

            CHECK_NODE(member_access);

            member_access->flags |= (LVALUE | CALLABLE);

            left = member_access;
        } else if (current_token->code == TOK_LSQ) {
            advance();

            struct node* expression;
            CHECK(parse_expression(parser, &expression, ctx), "failed to parse expression");

            current_token = get_current_token();

            EXPECT_TOKEN(current_token->code, TOK_RSQ);
            advance();

            struct node* index_access = node_new(NODE_INDEX, NULL, left, expression);
            CHECK_NODE(index_access);

            index_access->flags |= (LVALUE | CALLABLE);

            left = index_access;
        } else {
            break;
        }

        current_token = get_current_token();
    }
    *root = left;
    return 0;
}

static int parse_unary(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    if (current_token->code == TOK_NOT) {
        advance();

        struct node* not_unary;
        CHECK(parse_unary(parser, &not_unary, ctx), "failed to parse unary");

        struct node* not_node = node_new(NODE_NOT, NULL, not_unary, NULL);
        CHECK_NODE(not_node);

        *root = not_node;

        return 0;
    }

    CHECK(parse_postfix(parser, root, ctx), "failed to parse postfix");
    return 0;
}

static int parse_multiplicative(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left;
    CHECK(parse_unary(parser, &left, ctx), "failed to parse unary");

    const struct token* current_token = get_current_token();
    while (
        current_token->code == TOK_MUL  ||
        current_token->code == TOK_DIV
    ) {
        struct token operator_token = *current_token;
        advance();

        struct node* right;
        CHECK(parse_unary(parser, &right, ctx), "failed to parse unary");

        struct node* bin_op = node_new(NODE_BINARY_OP, &operator_token, left, right);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_current_token();
    }

    *root = left;

    return 0;
}

static int parse_additive(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left;
    CHECK(parse_multiplicative(parser, &left, ctx), "failed to parse multiplicative");

    const struct token* current_token = get_current_token();
    while (
        current_token->code == TOK_ADD  ||
        current_token->code == TOK_MIN
    ) {

        struct token operator_token = *current_token;
        advance();

        struct node* right;
        CHECK(parse_multiplicative(parser, &right, ctx), "failed to parse multiplicative");

        struct node* bin_op = node_new(NODE_BINARY_OP, &operator_token, left, right);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_current_token();
    }

    *root = left;

    return 0;
}

static int parse_relational(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left;
    CHECK(parse_additive(parser, &left, ctx), "failed to parse additive");

    const struct token* current_token = get_current_token();
    while (
        current_token->code == TOK_LES  ||
        current_token->code == TOK_LEQ  ||
        current_token->code == TOK_GRE  ||
        current_token->code == TOK_GRQ
    ) {
        struct token operator_token = *current_token;
        advance();

        struct node* right;
        CHECK(parse_additive(parser, &right, ctx), "failed to parse additive");

        struct node* bin_op = node_new(NODE_BINARY_OP, &operator_token, left, right);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_current_token();
    }

    *root = left;

    return 0;
}

static int parse_equality(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left;
    CHECK(parse_relational(parser, &left, ctx), "failed to parse relational");

    const struct token* current_token = get_current_token();
    while (current_token->code == TOK_DEQ || current_token->code == TOK_NEQ) {
        struct token operator_token = *current_token;
        advance();

        struct node* right;
        CHECK(parse_relational(parser, &right, ctx), "failed to parse relational");

        struct node* bin_op = node_new(NODE_BINARY_OP, &operator_token, left, right);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_current_token();
    }

    *root = left;

    return 0;
}

static int parse_and(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left;
    CHECK(parse_equality(parser, &left, ctx), "failed to parse equality");

    const struct token* current_token = get_current_token();
    while (current_token->code == TOK_AND) {
        struct token operator_token = *current_token;
        advance();

        struct node* right;
        CHECK(parse_equality(parser, &right, ctx), "failed to parse equality");

        struct node* bin_op = node_new(NODE_BINARY_OP, &operator_token, left, right);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_current_token();
    }

    *root = left;

    return 0;
}

static int parse_or(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* left;
    CHECK(parse_and(parser, &left, ctx), "failed to parse and");

    const struct token* current_token = get_current_token();
    while (current_token->code == TOK_OR) {
        struct token operator_token = *current_token;
        advance();

        struct node* right;
        CHECK(parse_and(parser, &right, ctx), "failed to parse and");

        struct node* bin_op = node_new(NODE_BINARY_OP, &operator_token, left, right);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_current_token();
    }

    *root = left;

    return 0;
}

static int parse_expression(struct parser* parser, struct node** root, struct context* ctx) {
    CHECK(parse_or(parser, root, ctx), "failed to parse or");
    return 0;
}

static int parse_expression_statement(struct parser* parser, struct node** root, struct context* ctx) {
    struct node* expression;
    CHECK(parse_expression(parser, &expression, ctx), "Failed to pare expression");

    const struct token* current_token = get_current_token();
    if (current_token->code == TOK_EQL &&  (expression->flags & LVALUE)) {
        advance();

        struct node* assignment_value;
        CHECK(parse_expression(parser, &assignment_value, ctx), "failed to parse expression");

        struct node* assignment_node = node_new(NODE_ASSIGN, NULL, expression, assignment_value);
        CHECK_NODE(assignment_node);

        *root = assignment_node;
        return 0;
    }

    struct node* expression_statement_node = node_new(NODE_EXP_STATEMENT, NULL, expression, NULL);
    CHECK_NODE(expression_statement_node);

    *root = expression_statement_node;
    return 0;
}

static int parse_block(struct parser* parser, struct node** root, struct context* ctx, bool brackets);

static int parse_if(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_IF)
    advance();

    struct node* if_expression;
    CHECK(parse_expression(parser, &if_expression, ctx), "failed to parse expression");

    struct node* true_side;
    CHECK(parse_block(parser, &true_side, ctx, true), "failed to parse block");

    struct node* false_side = NULL;
    current_token = get_current_token();
    if (current_token->code == TOK_ELS) {
        advance();

        CHECK(parse_block(parser, &false_side, ctx, true), "failed to parse block");
    }

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, false_side);
    CHECK_NODE(decision);

    struct node* if_node = node_new(NODE_IF, NULL, if_expression, decision);
    CHECK_NODE(if_node);

    *root = if_node;
    return 0;
}

static int parse_while(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_WHL);
    advance();

    struct node* while_expression;
    CHECK(parse_expression(parser, &while_expression, ctx), "failed to parse expression");

    struct node* true_side;
    CHECK(parse_block(parser, &true_side, ctx, true), "failed to parse block");

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, NULL);
    CHECK_NODE(decision);

    struct node* while_node = node_new(NODE_WHILE, NULL, while_expression, decision);
    CHECK_NODE(while_node);

    *root = while_node;

    return 0;
}


static int parse_parameter_list(struct parser* parser, struct node** root, struct context* ctx) {
    (void) ctx;

    const struct token* current_token = get_current_token();
    EXPECT_TOKEN(current_token->code, TOK_LPR);
    advance();

    struct node* parameters = NULL;

    current_token = get_current_token();

    while (current_token->code != TOK_RPR) {
        EXPECT_TOKEN(current_token->code, TOK_IDN);
        const struct token parameter_name = *current_token;
        advance();

        current_token = get_current_token();
        if (current_token->code != TOK_RPR) {
            EXPECT_TOKEN(current_token->code, TOK_COM);
            advance();
            current_token = get_current_token();
        }

        parameters = node_new(NODE_PARAMETER, &parameter_name, NULL, parameters);
        CHECK_NODE(parameters);
    }

    advance();

    *root = parameters;
    return 0;
}

static int parse_class(struct parser* parser, struct node** root, struct context* ctx);
static int parse_function(struct parser* parser, struct node** root, struct context* ctx);
static int parse_constant(struct parser* parser, struct node** root, struct context* ctx);

static int parse_export(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_EXP);
    advance();

    struct node* export_value = NULL;
    current_token = get_current_token();

    if (current_token->code == TOK_CLS) {
        CHECK(parse_class(parser, &export_value, ctx), "failed to parse class");
    } else if (current_token->code == TOK_FUN) {
        CHECK(parse_function(parser, &export_value, ctx), "failed to parse function");
    } else if (current_token->code == TOK_CON) {
        CHECK(parse_constant(parser, &export_value, ctx), "failed to parse constant");
    } else {
        ERROR("can't use export before token: %s", rev_tokens[current_token->code]);
        return 1;
    }

    struct node* export = node_new(NODE_EXPORT, NULL, export_value, NULL);
    CHECK_NODE(export);

    *root = export;
    return 0;
}

static int parse_function(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_FUN);
    advance();

    const struct token function_name = *get_current_token();
    EXPECT_TOKEN(function_name.code, TOK_IDN);
    advance();

    struct node* arguments;
    CHECK(parse_parameter_list(parser, &arguments, ctx), "failed to parse parameter list");

    struct node* body;
    CHECK(parse_block(parser, &body, ctx, true), "failed to parse block");

    struct node* function = node_new(NODE_FUNCTION, &function_name, arguments, body);
    CHECK_NODE(function);

    *root = function;
    return 0;
}

static int parse_return(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_RET);
    advance();

    // empty return only if last thing  from block
    struct node* expression = NULL;
    if (get_current_token()->code != TOK_RBR) {
        CHECK(parse_expression(parser, &expression, ctx), "failed to parse expression");
    }

    struct node* return_node = node_new(NODE_RETURN, NULL, expression, NULL);
    CHECK_NODE(return_node);

    *root = return_node;

    return 0;
}

static int parse_declaration(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_VAR);
    advance();

    current_token = get_current_token();
    EXPECT_TOKEN(current_token->code, TOK_IDN);
    advance();

    const struct token identifier = *current_token;

    current_token = get_current_token();
    struct node* init_value = NULL;
    if (current_token->code == TOK_EQL) {
        advance();
        CHECK(parse_expression(parser, &init_value, ctx), "failed to parse expression");
    }

    struct node* declaration = node_new(NODE_DECLARATION, &identifier, init_value, NULL);
    CHECK_NODE(declaration);

    *root = declaration;
    return 0;
}

static int parse_class(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_CLS);
    advance();

    current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_IDN);
    const struct token class_name = *current_token;
    advance();

    current_token = get_current_token();
    EXPECT_TOKEN(current_token->code, TOK_LBR);
    advance();

    current_token = get_current_token();

    struct node* members = NULL;
    while (current_token->code == TOK_VAR) {
        advance();

        current_token = get_current_token();
        EXPECT_TOKEN(current_token->code, TOK_IDN);

        members = node_new(NODE_MEMBER, current_token, NULL, members);
        CHECK_NODE(members);

        advance();
        current_token = get_current_token();
    }

    struct node* methods = NULL;
    while (current_token->code == TOK_FUN) {
        struct node* method;
        CHECK(parse_function(parser, &method, ctx), "failed to parse method");
        method->type = NODE_METHOD;

        methods = node_new(NODE_METHODS, NULL, method, methods);
        CHECK_NODE(methods);

        current_token = get_current_token();
    }

    EXPECT_TOKEN(current_token->code, TOK_RBR);
    advance();

    struct node* class_node = node_new(NODE_CLASS, &class_name, members, methods);
    CHECK_NODE(class_node);

    *root = class_node;
    return 0;
}

static int parse_constant(struct parser* parser, struct node** root, struct context* ctx) {
    const struct token* current_token = get_current_token();

    EXPECT_TOKEN(current_token->code, TOK_CON);
    advance();

    current_token = get_current_token();
    EXPECT_TOKEN(current_token->code, TOK_IDN);
    advance();

    const struct token identifier = *current_token;

    current_token = get_current_token();
    struct node* const_value = NULL;
    if (current_token->code == TOK_EQL) {
        advance();
        CHECK(parse_literal(parser, &const_value, ctx), "failed to parse literal");
    }

    struct node* declaration = node_new(NODE_CONSTANT, &identifier, const_value, NULL);
    CHECK_NODE(declaration);

    *root = declaration;
    return 0;

    return 0;
}

static int parse_import(struct parser* parser, struct node** root, struct context* ctx) {
    (void) ctx;

    const struct token* current_token = get_current_token();
    EXPECT_TOKEN(current_token->code, TOK_IMP);
    advance();

    current_token = get_current_token();
    EXPECT_TOKEN(current_token->code, TOK_STR);
    advance();

    struct node* block = NULL;
    CHECK(parse(parser, &block), "failed to parse module");

    struct node* import_node = node_new(NODE_IMPORT, NULL, block, NULL);
    CHECK_NODE(import_node);

    *root = import_node;
    return 0;
}

static int parse_statement(struct parser* parser, struct node** root, struct context* ctx) {
    int current_token_code = get_current_token()->code;

    if (current_token_code == TOK_IF) {
        CHECK(parse_if(parser, root, ctx), "failed to parse if statement");
        return 0;
    }

    if (current_token_code == TOK_WHL) {
        CHECK(parse_while(parser, root, ctx), "failed to parse while statement");
        return 0;
    }

    if (current_token_code == TOK_FUN) {
        CHECK(parse_function(parser, root, ctx), "failed to parse function");
        return 0;
    }

    if (current_token_code == TOK_VAR) {
        CHECK(parse_declaration(parser, root, ctx), "failed to parse declaration");
        return 0;
    }

    if (current_token_code == TOK_IMP) {
        CHECK(parse_import(parser, root, ctx), "failed to parse import");
        return 0;
    }

    if (current_token_code == TOK_EXP) {
        CHECK(parse_export(parser, root, ctx), "failed to parse export");
        return 0;
    }

    if (current_token_code == TOK_CON) {
        CHECK(parse_constant(parser, root, ctx), "failed to parse declaration");
        return 0;
    }

    if (current_token_code == TOK_CLS) {
        CHECK(parse_class(parser, root, ctx), "failed to parse class");
        return 0;
    }

    if (current_token_code == TOK_RET) {
        CHECK(parse_return(parser, root, ctx), "failed to parse return");
        return 0;
    }

    CHECK(parse_expression_statement(parser, root, ctx), "failed to parse expression statement");
    return 0;
}

static int parse_block(struct parser* parser, struct node** root, struct context* ctx, bool brackets) {
    const struct token* current_token = get_current_token();

    if (brackets) {
        EXPECT_TOKEN(current_token->code, TOK_LBR);
        advance();
    }

    struct node* statements = NULL;
    while (current_token->code != TOK_RBR && current_token->code != TOK_EOF) {
        struct node* statement;
        CHECK(parse_statement(parser, &statement, ctx), "failed to parse statement");

        statements = node_new(NODE_STATEMENT, NULL, statement, statements);
        CHECK_NODE(statements);

        current_token = get_current_token();
    }

    if (brackets) {
        EXPECT_TOKEN(current_token->code, TOK_RBR);
        advance();
    } else {
        EXPECT_TOKEN(current_token->code, TOK_EOF);
    }

    struct node* block = node_new(NODE_BLOCK, NULL, statements, NULL);
    CHECK_NODE(block);

    *root = block;
    return 0;
}

int parse(struct parser* parser, struct node** root) {
    struct context ctx;

    advance();
    CHECK(parse_block(parser, root, &ctx, false), "failed to parse block");
    return 0;
}
