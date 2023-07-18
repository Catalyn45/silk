#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "utils.h"

#define get_token() \
    (&parser->tokens[parser->current_index])

#define advance() \
    (++parser->current_index)

static int parse_expression(struct parser* parser, struct node** root);
static int parse_function_call(struct parser* parser, struct node** root);

static int parse_factor(struct parser* parser, struct node** root) {
    const struct token_entry* current_token = get_token();

    if (current_token->code == TOK_NOT) {
        // eat ! keyword
        advance();

        struct node* not_expression;
        int result = parse_expression(parser, &not_expression);
        if (result != 0) {
            ERROR("failed to parse expression");
            return result;
        }

        struct node* not_node = node_new(NODE_NOT, NULL, not_expression, NULL, parser->current_scope);
        if (!not_node) {
            ERROR("memory allocation failed");
            return 1;
        }

        *root = not_node;

        return 0;
    }

    if (current_token->code == TOK_LPR) {
        // eat left par
        advance();

        int result = parse_expression(parser, root);
        if (result != 0) {
            ERROR("failed to parse expression");
            return result;
        }

        current_token = get_token();
        if (current_token->code != TOK_RPR) {
            EXPECTED_TOKEN(TOK_RPR, current_token->code);
            return 1;
        }

        // eat right par
        advance();
        return 0;
    }

    //TODO: function call is expression

    if (current_token->code == TOK_IDN) {
        if (parser->tokens[(parser->current_index) + 1].code == TOK_LPR) {
            int res = parse_function_call(parser, root);
            if (res != 0) {
                ERROR("failed to parse function call");
            }

            return res;
        }
    }

    if (current_token->code != TOK_INT && current_token->code != TOK_IDN) {
        ERROR("expected token code: int or identifier, got token: %s", rev_tokens[current_token->code]);
        return 1;
    }

    struct node* node_num = node_new(current_token->code == TOK_INT ? NODE_NUMBER : NODE_VAR, current_token, NULL, NULL, parser->current_scope);
    if (!node_num) {
        ERROR("memory allocation failed");
        return 1;
    }

    // eat current token
    advance();

    *root = node_num;

    return 0;
}

static int parse_term(struct parser* parser, struct node** root) {
    struct node* left;
    int result = parse_factor(parser, &left);
    if ( result != 0 ) {
        ERROR("failed to parse factor");
        return 1;
    }

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
        result = parse_factor(parser, &right);
        if ( result != 0 ) {
            ERROR("failed to parse factor");
            return 1;
        }

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        if (!bin_op) {
            ERROR("memory allocation failed");
            return 1;
        }

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_expression(struct parser* parser, struct node** root) {
    struct node* left;
    int result = parse_term(parser, &left);
    if ( result != 0 ) {
        ERROR("failed to parse term");
        return 1;
    }

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
        result = parse_term(parser, &right);
        if ( result != 0 ) {
            ERROR("failed to parse term");
            return 1;
        }

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right, parser->current_scope);
        if (!bin_op) {
            ERROR("memory allocation failed");
            return 1;
        }

        left = bin_op;
        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_block(struct parser* parser, struct node** root, bool brackets);

static int parse_if(struct parser* parser, struct node** root ) {
    // eat if keyword
    advance();

    struct node* if_expression;
    int res = parse_expression(parser, &if_expression);
    if (res != 0) {
        ERROR("failed to parse expression");
        return res;
    }

    struct node* true_side;
    res = parse_block(parser, &true_side, true);
    if (res != 0) {
        ERROR("failed to parse statements");
        return res;
    }

    struct node* false_side = NULL;
    const struct token_entry* current_token = get_token();
    if (current_token->code == TOK_ELS) {
        // eat else keyword
        advance();
        int res = parse_block(parser, &false_side, true);
        if (res != 0) {
            ERROR("failed to parse statements");
            return res;
        }
    }

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, false_side, parser->current_scope);
    if (!decision) {
        ERROR("failed allocating memory");
        return 1;
    }

    struct node* if_node = node_new(NODE_IF, NULL, if_expression, decision, parser->current_scope);
    if (!if_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = if_node;
    return 0;
}

static int parse_while(struct parser* parser, struct node** root ) {
    // eat while keyword
    advance();

    struct node* while_expression;
    int res = parse_expression(parser, &while_expression);
    if (res != 0) {
        ERROR("failed to parse expression");
        return res;
    }

    struct node* true_side;
    res = parse_block(parser, &true_side, true);
    if (res != 0) {
        ERROR("failed to parse statements");
        return res;
    }

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, NULL, parser->current_scope);
    if (!decision) {
        ERROR("failed allocating memory");
        return 1;
    }

    struct node* while_node = node_new(NODE_WHILE, NULL, while_expression, decision, parser->current_scope);
    if (!while_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = while_node;

    return 0;
}

static int parse_assignment(struct parser* parser, struct node** root ) {
    const struct token_entry* variable_token = get_token();

    // eat variable name
    advance();

    const struct token_entry* current_token = get_token();
    if (current_token->code != TOK_EQL) {
        EXPECTED_TOKEN(TOK_EQL, current_token->code);
        return 1;
    }

    // eat equals
    advance();

    struct node* var_node = node_new(NODE_VAR, variable_token, NULL, NULL, parser->current_scope);
    if (!var_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    struct node* assignment_value;
    int res = parse_expression(parser, &assignment_value);
    if (res != 0) {
        ERROR("failed to parse expression");
        return 1;
    }

    struct node* assignment_node = node_new(NODE_ASSIGN, NULL, var_node, assignment_value, parser->current_scope);
    if (!assignment_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = assignment_node;
    return 0;
}

static int parse_argument_list(struct parser* parser, struct node** root ) {
    const struct token_entry* current_token = get_token();
    if (current_token->code != TOK_LPR) {
        EXPECTED_TOKEN(TOK_LPR, current_token->code);
        return 1;
    }

    struct node* argument_list = NULL;

    struct node first_arg = {0};
    struct node* argument = &first_arg;

    do {

        // eat left par or comma
        advance();

        current_token = get_token();
        if (current_token->code != TOK_COM && current_token->code != TOK_RPR) {
            argument->right = node_new(NODE_ARGUMENT, NULL, NULL, NULL, parser->current_scope);
            if (!argument->right) {
                ERROR("failed allocating memory");
                return 1;
            }

            int res = parse_expression(parser, &argument->right->left);
            if (res != 0) {
                ERROR("failed to parse expression");
                return res;
            }

            if (!argument_list) {
                argument_list = argument;
            }

            argument = argument->right;
            current_token = get_token();
        }

    } while (current_token->code == TOK_COM );

    current_token = get_token();
    if (current_token->code != TOK_RPR) {
        EXPECTED_TOKEN(TOK_RPR, current_token->code);
        return 1;
    }

    // eat right par
    advance();

    *root = argument_list->right;
    return 0;
}

static int parse_function_call(struct parser* parser, struct node** root ) {
    const struct token_entry* function_name = get_token();

    // eat function name
    advance();

    struct node* function_parameters = NULL;
    int res = parse_argument_list(parser, &function_parameters);
    if (res != 0) {
        ERROR("failed to parse parameter list");
        return 1;
    }

    struct node* function_node = node_new(NODE_FUNCTION_CALL, function_name, function_parameters, NULL, parser->current_scope);
    if (!function_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = function_node;
    return 0;
}

static int parse_parameter_list(struct parser* parser, struct node** root ) {
    const struct token_entry* current_token = get_token();
    if (current_token->code != TOK_LPR) {
        EXPECTED_TOKEN(TOK_LPR, current_token->code);
        return 1;
    }

    struct node* parameter_list = NULL;

    struct node first_par = {0};
    struct node* parameter = &first_par;

    do {
        // eat left par or comma
        advance();

        current_token = get_token();
        if (current_token->code == TOK_IDN) {
            parameter->left = node_new(NODE_PARAMETER, current_token, NULL, NULL, parser->current_scope);
            if (!parameter) {
                ERROR("failed allocating memory");
                return 1;
            }

            if (!parameter_list) {
                parameter_list = parameter;
            }

            // TODO: fix here
            parameter = parameter->left;
            advance();
        }

    } while (get_token()->code == TOK_COM );

    current_token = get_token();
    if (current_token->code != TOK_RPR) {
        EXPECTED_TOKEN(TOK_RPR, current_token->code);
        return 1;
    }

    // eat right par
    advance();

    *root = parameter_list->left;
    return 0;
}

static int parse_function(struct parser* parser, struct node** root ) {
    // eat def keyword
    advance();

    const struct token_entry* function_name = get_token();
    if (function_name->code != TOK_IDN) {
        EXPECTED_TOKEN(TOK_IDN, function_name->code);
        return 1;
    }

    // eat function name
    advance();

    struct node* arguments;
    int res = parse_parameter_list(parser, &arguments);
    if (res != 0) {
        ERROR("failed to parse argument list");
        return res;
    }

    struct node* body;
    res = parse_block(parser, &body, true);
    if (res != 0) {
        ERROR("failed to parse statements");
        return res;
    }

    struct node* function = node_new(NODE_FUNCTION, function_name, arguments, body, parser->current_scope);
    if (!function) {
        ERROR("memory allocating failed");
        return 1;
    }

    *root = function;
    return 0;
}

static int parse_return(struct parser* parser, struct node** root ) {
    // eat return keyword
    advance();

    struct node* expression;
    int result = parse_expression(parser, &expression);
    if (result != 0) {
        ERROR("failed to parse expression");
        return result;
    }

    struct node* return_node = node_new(NODE_RETURN, NULL, expression, NULL, parser->current_scope);
    if (!return_node) {
        ERROR("memory allocating failed");
        return 1;
    }

    *root = return_node;

    return 0;
}

static int parse_statement(struct parser* parser, struct node** root ) {
    int current_token_code = get_token()->code;

    int res;
    if (current_token_code == TOK_IF) {
        res = parse_if(parser, root);
        if (res != 0) {
            ERROR("failed to parse if");
        }

        return res;
    }

    if (current_token_code == TOK_WHL) {
        res = parse_while(parser, root);
        if (res != 0) {
            ERROR("failed to parse while");
        }

        return res;
    }

    if (current_token_code == TOK_FUN) {
        res = parse_function(parser, root);
        if (res != 0) {
            ERROR("failed to parse function");
        }

        return res;
    }

    if (current_token_code == TOK_RET) {
        res = parse_return(parser, root);
        if (res != 0) {
            ERROR("failed to parse function");
        }

        return res;
    }


    if (current_token_code == TOK_IDN) {
        if (parser->tokens[(parser->current_index) + 1].code == TOK_EQL) {
            res = parse_assignment(parser, root);
            if (res != 0) {
                ERROR("failed to parse assignment");
            }

            return res;

        } else if (parser->tokens[(parser->current_index) + 1].code == TOK_LPR) {
            res = parse_function_call(parser, root);
            if (res != 0) {
                ERROR("failed to parse function call");
            }

            return res;
        }
    }

    res = parse_expression(parser, root);
    if (res != 0) {
        ERROR("failed to parse expression");
    }

    return res;
}

static int parse_block(struct parser* parser, struct node** root, bool brackets) {
    const struct token_entry* current_token = get_token();

    if (brackets) {
        if (current_token->code != TOK_LBR) {
            EXPECTED_TOKEN(TOK_LBR, current_token->code);
            return 1;
        }

        // eat left bracket
        advance();

        ++parser->current_scope;
    }


    struct node* first_statement = NULL;
    struct node* statement = NULL;

    while (current_token->code != TOK_RBR && current_token->code != TOK_EOF) {
        if (!statement) {
            statement = node_new(NODE_STATEMENT, NULL, NULL, NULL, parser->current_scope);
            if (!statement) {
                ERROR("Error allocating memory");
                return 1;
            }
        } else {
            statement->right = node_new(NODE_STATEMENT, NULL, NULL, NULL, parser->current_scope);
            if (!statement->right) {
                ERROR("Error allocating memory");
                return 1;
            }
            statement = statement->right;
        }

        if (parse_statement(parser, &statement->left) != 0) {
            ERROR("failed to parse statement");
            return 1;
        }

        if (!first_statement) {
            first_statement = statement;
        }

        current_token = get_token();
    }

    free(statement->right);
    statement->right = NULL;

    if (brackets) {
        if (current_token->code != TOK_RBR) {
            EXPECTED_TOKEN(TOK_RBR, current_token->code);
            return 1;
        }

        // eat right bracket
        advance();

        --parser->current_scope;
    } else if (current_token->code != TOK_EOF) {
        EXPECTED_TOKEN(TOK_EOF, current_token->code);
        return 1;
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
