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
static int parse_function_call(struct parser* parser, struct node** root);

static int parse_factor(struct parser* parser, struct node** root) {
    const struct token_entry* current_token = get_token();

    if (current_token->code == TOK_NOT) {
        // eat ! keyword
        advance();

        struct node* not_expression;
        CHECK(parse_expression(parser, &not_expression), "failed to parse expression");

        struct node* not_node = node_new(NODE_NOT, NULL, not_expression, NULL, parser->current_scope);
        CHECK_NODE(not_node);

        *root = not_node;

        return 0;
    }

    if (current_token->code == TOK_LPR) {
        // eat left par
        advance();

        CHECK(parse_expression(parser, root), "failed to parse expression");

        current_token = get_token();
        EXPECT_TOKEN(current_token->code, TOK_RPR);

        // eat right par
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

    if (current_token->code == TOK_IDN) {
        if (next_token()->code == TOK_LPR) {
            CHECK(parse_function_call(parser, root), "failed to parse function call");
            return 0;
        }

        advance();
        struct node* node_var = node_new(NODE_VAR, current_token, NULL, NULL, parser->current_scope);
        CHECK_NODE(node_var);

        *root = node_var;
        return 0;
    }

    ERROR("unexpected token %s", rev_tokens[current_token->code]);
    return 1;
}

static int parse_term(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_factor(parser, &left), "failed to parse factor");

    const struct token_entry* current_token = get_token();
    while (
        current_token->code == TOK_MUL  ||
        current_token->code == TOK_DIV  ||
        current_token->code == TOK_LES  ||
        current_token->code == TOK_LEQ  ||
        current_token->code == TOK_GRE  ||
        current_token->code == TOK_GRQ  ||
        current_token->code == TOK_DEQ  ||
        current_token->code == TOK_NEQ
    ) {
        // eat current token
        advance();

        struct node* right;
        CHECK(parse_factor(parser, &right), "failed to parse factor");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_expression(struct parser* parser, struct node** root) {
    struct node* left;
    CHECK(parse_term(parser, &left), "failed to parse term");

    const struct token_entry* current_token = get_token();
    while (
        current_token->code == TOK_ADD  ||
        current_token->code == TOK_MIN  ||
        current_token->code == TOK_AND  ||
        current_token->code == TOK_OR
    ) {
        // eat current token
        advance();

        struct node* right;
        CHECK(parse_term(parser, &right), "failed to parse term");

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        CHECK_NODE(bin_op);

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_expression_statement(struct parser* parser, struct node** root) {
    struct node* expression;
    CHECK(parse_expression(parser, &expression), "Failed to pare expression");

    struct node* expression_statement_node = node_new(NODE_EXP_STATEMENT, NULL, expression, NULL, parser->current_scope);
    CHECK_NODE(expression_statement_node);

    *root = expression_statement_node;
    return 0;
}

static int parse_block(struct parser* parser, struct node** root, bool brackets);

static int parse_if(struct parser* parser, struct node** root ) {
    // eat if keyword
    advance();

    struct node* if_expression;
    CHECK(parse_expression(parser, &if_expression), "failed to parse expression");

    struct node* true_side;
    CHECK(parse_block(parser, &true_side, true), "failed to parse block");

    struct node* false_side = NULL;
    const struct token_entry* current_token = get_token();
    if (current_token->code == TOK_ELS) {
        // eat else keyword
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
    // eat while keyword
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

static int parse_assignment(struct parser* parser, struct node** root ) {
    const struct token_entry* variable_token = get_token();

    // eat variable name
    advance();

    const struct token_entry* current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_EQL);

    // eat equals
    advance();

    struct node* var_node = node_new(NODE_VAR, variable_token, NULL, NULL, parser->current_scope);
    CHECK_NODE(var_node);

    struct node* assignment_value;
    CHECK(parse_expression(parser, &assignment_value), "failed to parse expression");

    struct node* assignment_node = node_new(NODE_ASSIGN, NULL, var_node, assignment_value, parser->current_scope);
    CHECK_NODE(assignment_node);

    *root = assignment_node;
    return 0;
}

static int parse_argument_list(struct parser* parser, struct node** root ) {
    const struct token_entry* current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_LPR);

    struct node* argument_list = NULL;

    struct node first_arg = {0};
    struct node* argument = &first_arg;

    do {

        // eat left par or comma
        advance();

        current_token = get_token();
        if (current_token->code != TOK_COM && current_token->code != TOK_RPR) {
            argument->right = node_new(NODE_ARGUMENT, NULL, NULL, NULL, parser->current_scope);
            CHECK_NODE(argument->right);

            CHECK(parse_expression(parser, &argument->right->left), "false to parse expression");

            if (!argument_list) {
                argument_list = argument;
            }

            argument = argument->right;
            current_token = get_token();
        }

    } while (current_token->code == TOK_COM );

    current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_RPR);

    // eat right par
    advance();

    *root = argument_list ? argument_list->right : NULL;
    return 0;
}

static int parse_function_call(struct parser* parser, struct node** root ) {
    const struct token_entry* function_name = get_token();

    // eat function name
    advance();

    struct node* function_parameters = NULL;
    CHECK(parse_argument_list(parser, &function_parameters), "failed to parse argument list");

    struct node* function_node = node_new(NODE_FUNCTION_CALL, function_name, function_parameters, NULL, parser->current_scope);
    CHECK_NODE(function_node);

    *root = function_node;
    return 0;
}

static int parse_parameter_list(struct parser* parser, struct node** root ) {
    const struct token_entry* current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_LPR);

    struct node* parameter_list = NULL;

    struct node first_par = {0};
    struct node* parameter = &first_par;

    do {
        // eat left par or comma
        advance();

        current_token = get_token();
        if (current_token->code == TOK_IDN) {
            parameter->left = node_new(NODE_PARAMETER, current_token, NULL, NULL, parser->current_scope);
            CHECK_NODE(parameter->left);

            if (!parameter_list) {
                parameter_list = parameter;
            }

            // TODO: fix here
            parameter = parameter->left;
            advance();
        }

    } while (get_token()->code == TOK_COM );

    current_token = get_token();
    EXPECT_TOKEN(current_token->code, TOK_RPR);

    // eat right par
    advance();

    *root = parameter_list ? parameter_list->left : NULL;
    return 0;
}

static int parse_function(struct parser* parser, struct node** root ) {
    // eat def keyword
    advance();

    const struct token_entry* function_name = get_token();
    EXPECT_TOKEN(function_name->code, TOK_IDN);

    // eat function name
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
    // eat return keyword
    advance();

    struct node* expression;
    CHECK(parse_expression(parser, &expression), "failed to parse expression");

    struct node* return_node = node_new(NODE_RETURN, NULL, expression, NULL, parser->current_scope);
    CHECK_NODE(return_node);

    *root = return_node;

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

    if (current_token_code == TOK_RET) {
        CHECK(parse_return(parser, root), "failed to parse return");
        return 0;
    }

    if (current_token_code == TOK_IDN && next_token()->code == TOK_EQL) {
        CHECK(parse_assignment(parser, root), "failed to parse assignment");
        return 0;
    }

    CHECK(parse_expression_statement(parser, root), "failed to parse expression statement");
    return 0;
}

static int parse_block(struct parser* parser, struct node** root, bool brackets) {
    const struct token_entry* current_token = get_token();

    if (brackets) {
        EXPECT_TOKEN(current_token->code, TOK_LBR);

        // eat left bracket
        advance();

        ++parser->current_scope;
    }


    struct node* first_statement = NULL;
    struct node* statement = NULL;

    while (current_token->code != TOK_RBR && current_token->code != TOK_EOF) {
        if (!statement) {
            statement = node_new(NODE_STATEMENT, NULL, NULL, NULL, parser->current_scope);
            CHECK_NODE(statement)
        } else {
            statement->right = node_new(NODE_STATEMENT, NULL, NULL, NULL, parser->current_scope);
            CHECK_NODE(statement->right);

            statement = statement->right;
        }

        CHECK(parse_statement(parser, &statement->left), "failed to parse statement");

        if (!first_statement) {
            first_statement = statement;
        }

        current_token = get_token();
    }

    if (statement) {
        free(statement->right);
        statement->right = NULL;
    }

    if (brackets) {
        EXPECT_TOKEN(current_token->code, TOK_RBR);

        // eat right bracket
        advance();

        --parser->current_scope;
    } else {
        EXPECT_TOKEN(current_token->code, TOK_EOF);
    }

    *root = first_statement;
    return 0;
}

int parse(struct parser* parser, struct node** root) {
    uint32_t current_index = 0;
    int res = parse_block(parser, root, false);
    if (res != 0) {
        const struct token_entry* token = &parser->tokens[current_index];
        ERROR("at line %d", token->line);
        print_program_error(parser->text, token->index);
    }

    return res;
}
