#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "utils.h"

#define get_token() \
    (&tokens[*current_index])

#define advance() \
    (++(*current_index))

static int parse_expression(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root);

static int parse_factor(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    const struct token_entry* current_token = get_token();

    if (current_token->code == TOK_NOT) {
        // eat ! keyword
        advance();

        struct node* not_expression;
        int result = parse_expression(current_index, tokens, n_tokens, &not_expression);
        if (result != 0) {
            ERROR("failed to parse expression");
            return result;
        }

        struct node* not_node = node_new(NODE_NOT, NULL, not_expression, NULL);
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

        int result = parse_expression(current_index, tokens, n_tokens, root);
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

    if (current_token->code != TOK_INT && current_token->code != TOK_IDN) {
        ERROR("expected token code: int or identifier, got token: %s", rev_tokens[current_token->code]);
        return 1;
    }

    struct node* node_num = node_new(current_token->code == TOK_INT ? NODE_NUMBER : NODE_VAR, current_token, NULL, NULL);
    if (!node_num) {
        ERROR("memory allocation failed");
        return 1;
    }

    // eat current token
    advance();

    *root = node_num;

    return 0;
}

static int parse_term(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* left;
    int result = parse_factor(current_index, tokens, n_tokens, &left);
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
        result = parse_factor(current_index, tokens, n_tokens, &right);
        if ( result != 0 ) {
            ERROR("failed to parse factor");
            return 1;
        }

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right);
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

static int parse_expression(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* left;
    int result = parse_term(current_index, tokens, n_tokens, &left);
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
        result = parse_term(current_index, tokens, n_tokens, &right);
        if ( result != 0 ) {
            ERROR("failed to parse term");
            return 1;
        }

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, left, right);
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

static int parse_statements(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root, bool brackets);

static int parse_if(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    // eat if keyword
    advance();

    struct node* if_expression;
    int res = parse_expression(current_index, tokens, n_tokens, &if_expression);
    if (res != 0) {
        ERROR("failed to parse expression");
        return res;
    }

    struct node* true_side;
    res = parse_statements(current_index, tokens, n_tokens, &true_side, true);
    if (res != 0) {
        ERROR("failed to parse statements");
        return res;
    }

    struct node* false_side = NULL;
    const struct token_entry* current_token = get_token();
    if (current_token->code == TOK_ELS) {
        // eat else keyword
        advance();
        int res = parse_statements(current_index, tokens, n_tokens, &false_side, true);
        if (res != 0) {
            ERROR("failed to parse statements");
            return res;
        }
    }

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, false_side);
    if (!decision) {
        ERROR("failed allocating memory");
        return 1;
    }

    struct node* if_node = node_new(NODE_IF, NULL, if_expression, decision);
    if (!if_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = if_node;
    return 0;
}

static int parse_while(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    // eat while keyword
    advance();

    struct node* while_expression;
    int res = parse_expression(current_index, tokens, n_tokens, &while_expression);
    if (res != 0) {
        ERROR("failed to parse expression");
        return res;
    }

    struct node* true_side;
    res = parse_statements(current_index, tokens, n_tokens, &true_side, true);
    if (res != 0) {
        ERROR("failed to parse statements");
        return res;
    }

    struct node* decision = node_new(NODE_DECISION, NULL, true_side, NULL);
    if (!decision) {
        ERROR("failed allocating memory");
        return 1;
    }

    struct node* while_node = node_new(NODE_WHILE, NULL, while_expression, decision);
    if (!while_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = while_node;

    return 0;
}

static int parse_assignment(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
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

    struct node* var_node = node_new(NODE_VAR, variable_token, NULL, NULL);
    if (!var_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    struct node* assignment_value;
    int res = parse_expression(current_index, tokens, n_tokens, &assignment_value)    ;
    if (res != 0) {
        ERROR("failed to parse expression");
        return 1;
    }

    struct node* assignment_node = node_new(NODE_ASSIGN, NULL, var_node, assignment_value);
    if (!assignment_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = assignment_node;
    return 0;
}

static int parse_function_call(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    const struct token_entry* function_name = get_token();

    // eat function name
    advance();

    const struct token_entry* current_token = get_token();
    if (current_token->code != TOK_LPR) {
        EXPECTED_TOKEN(TOK_LPR, current_token->code);
        return 1;
    }

    // eat left par
    advance();

    struct node* function_parameter;
    int res = parse_expression(current_index, tokens, n_tokens, &function_parameter);
    if (res != 0) {
        ERROR("failed to parse expression");
        return 1;
    }

    current_token = get_token();
    if (current_token->code != TOK_RPR) {
        EXPECTED_TOKEN(TOK_RPR, current_token->code);
        return 1;
    }

    // eat right par
    advance();

    struct node* function_node = node_new(NODE_FUNCTION_CALL, function_name, function_parameter, NULL);
    if (!function_node) {
        ERROR("memory allocation failed");
        return 1;
    }

    *root = function_node;
    return 0;
}

static int parse_argument_list(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    (void) tokens;
    (void) n_tokens;
    (void) root;

    const struct token_entry* current_token = get_token();
    if (current_token->code != TOK_LPR) {
        EXPECTED_TOKEN(TOK_LPR, current_token->code);
        return 1;
    }

    while (current_token->code == TOK_COM ) {

    }

    current_token = get_token();
    if (current_token->code != TOK_RPR) {
        EXPECTED_TOKEN(TOK_RPR, current_token->code);
        return 1;
    }

    return 0;
}

static int parse_function(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    // eat def keyword
    advance();

    const struct token_entry* current_token = get_token();
    if (current_token->code != TOK_IDN) {
        EXPECTED_TOKEN(TOK_IDN, current_token->code);
        return 1;
    }

    // eat function name
    advance();

    struct node* arguments;
    int res = parse_argument_list(current_index, tokens, n_tokens, &arguments);
    if (res != 0) {
        ERROR("memory allocation failed");
        return res;
    }

    struct node* body;
    res = parse_statements(current_index, tokens, n_tokens, &body, true);
    if (res != 0) {
        ERROR("failed to parse statements");
        return res;
    }

    struct node* function = node_new(NODE_FUNCTION, current_token, arguments, body);
    if (!function) {
        ERROR("memory allocating failed");
        return 1;
    }

    *root = function;
    return 0;
}

static int parse_statement(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    int current_token_code = get_token()->code;

    int res;
    if (current_token_code == TOK_IF) {
        res = parse_if(current_index, tokens, n_tokens, root);
        if (res != 0) {
            ERROR("failed to parse if");
        }

        return res;
    }

    if (current_token_code == TOK_WHL) {
        res = parse_while(current_index, tokens, n_tokens, root);
        if (res != 0) {
            ERROR("failed to parse while");
        }

        return res;
    }

    if (current_token_code == TOK_FUN) {
        res = parse_function(current_index, tokens, n_tokens, root);
        if (res != 0) {
            ERROR("failed to parse function");
        }

        return res;
    }

    if (current_token_code == TOK_IDN) {
        if (tokens[(*current_index) + 1].code == TOK_EQL) {
            res = parse_assignment(current_index, tokens, n_tokens, root);
            if (res != 0) {
                ERROR("failed to parse assignment");
            }

            return res;

        } else if (tokens[(*current_index) + 1].code == TOK_LPR) {
            res = parse_function_call(current_index, tokens, n_tokens, root);
            if (res != 0) {
                ERROR("failed to parse function call");
            }

            return res;
        }
    }

    res = parse_expression(current_index, tokens, n_tokens, root);
    if (res != 0) {
        ERROR("failed to parse expression");
    }

    return res;
}

static int parse_statements(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root, bool brackets) {
    const struct token_entry* current_token = get_token();

    if (brackets) {
        if (current_token->code != TOK_LBR) {
            EXPECTED_TOKEN(TOK_LBR, current_token->code);
            return 1;
        }

        // eat left bracket
        advance();
    }

    struct node* first_statement = node_new(NODE_STATEMENT, NULL, NULL, NULL);
    if (!first_statement) {
        ERROR("Error allocating memory");
        return 1;
    }

    struct node* statement = first_statement;

    while (current_token->code != TOK_RBR && current_token->code != TOK_EOF) {
        if (parse_statement(current_index, tokens, n_tokens, &statement->left) != 0) {
            ERROR("failed to parse statement");
            return 1;
        }

        struct node* new_statement = node_new(NODE_STATEMENT, NULL, NULL, NULL);
        if (!new_statement) {
            ERROR("Error allocating memory");
            return 1;
        }

        statement->right = new_statement;
        statement = new_statement;
        current_token = get_token();
    }

    if (brackets) {
        if (current_token->code != TOK_RBR) {
            EXPECTED_TOKEN(TOK_RBR, current_token->code);
            return 1;
        }

        // eat right bracket
        advance();
    } else if (current_token->code != TOK_EOF) {
        EXPECTED_TOKEN(TOK_EOF, current_token->code);
        return 1;
    }

    *root = first_statement;
    return 0;
}

int parse(const char* text, const struct token_entry* tokens, uint32_t n_tokens, struct node** root ) {
    uint32_t current_index = 0;
    int res = parse_statements(&current_index, tokens, n_tokens, root, false);
    if (res != 0) {
        const struct token_entry* token = &tokens[current_index];
        ERROR("at line %d", token->line);
        print_program_error(text, token->index);
    }

    return res;
}
