#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include <stdlib.h>

#define get_token() \
    &tokens[*current_index]

#define advance() \
    ++(*current_index)

static int expression(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root);

static int factor(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    const struct token_entry* current_token = get_token();

    if (current_token->token_code == TOK_NOT) {
        advance();

        struct node* not_node = node_new(NODE_NOT, NULL, NULL, NULL, NULL);
        if (!not_node) {
            return 1;
        }

        int result = expression(current_index, tokens, n_tokens, &not_node->value);
        if (result != 0) {
            return result;
        }

        *root = not_node;

        return 0;
    }

    if (current_token->token_code == TOK_LPR) {
        advance();
        int result = expression(current_index, tokens, n_tokens, root);
        if (result != 0) {
            return result;
        }

        current_token = get_token();
        if (current_token->token_code != TOK_RPR) {
            return 1;
        }

        advance();
        return 0;
    }

    if (current_token->token_code != TOK_INT && current_token->token_code != TOK_IDN) {
        // TODO: handle
        return 1;
    }

    struct node* node_num = node_new(current_token->token_code == TOK_INT ? NODE_NUMBER : NODE_VAR, current_token, NULL, NULL, NULL);
    if (!node_num) {
        return 1;
    }

    advance();

    *root = node_num;

    return 0;
}

static int term(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* left = NULL;
    int result = factor(current_index, tokens, n_tokens, &left);
    if ( result != 0 ) {
        // TODO: handle
        return 1;
    }

    const struct token_entry* current_token = get_token();
    while (
        current_token->token_code == TOK_MUL  ||
        current_token->token_code == TOK_DIV  ||
        current_token->token_code == TOK_LES  ||
        current_token->token_code == TOK_LEQ  ||
        current_token->token_code == TOK_GRE  ||
        current_token->token_code == TOK_GRQ  ||
        current_token->token_code == TOK_DEQ
    ) {
        advance();
        struct node* right = NULL;
        result = factor(current_index, tokens, n_tokens, &right);
        if ( result != 0 ) {
            // TODO: handle
            return 1;
        }

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, NULL, left, right);
        if (!bin_op) {
            // TODO: handle
            return 1;
        }

        left = bin_op;

        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int expression(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* left = NULL;
    int result = term(current_index, tokens, n_tokens, &left);
    if ( result != 0 ) {
        // TODO: handle
        return 1;
    }

    const struct token_entry* current_token = get_token();
    while (
        current_token->token_code == TOK_ADD  ||
        current_token->token_code == TOK_MIN  ||
        current_token->token_code == TOK_AND  ||
        current_token->token_code == TOK_OR
    ) {
        advance();
        struct node* right = NULL;
        result = term(current_index, tokens, n_tokens, &right);
        if ( result != 0 ) {
            // TODO: handle
            return 1;
        }

        struct node* bin_op = node_new(NODE_BINARY_OP, current_token, NULL, left, right);
        if (!bin_op) {
            // TODO: handle
            return 1;
        }

        left = bin_op;

        current_token = get_token();
    }

    *root = left;

    return 0;
}

static int parse_statements(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root);

static int parse_if(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* true_side = NULL;

    advance();

    struct node* value = NULL;
    int res = expression(current_index, tokens, n_tokens, &value);
    if (tokens[*current_index].token_code != TOK_LBR) {
        // TODO: handle
        return 1;
    }

    advance();
    res = parse_statements(current_index, tokens, n_tokens, &true_side);
    if (res != 0) {
        return res;
    }

    if (tokens[*current_index].token_code != TOK_RBR) {
        // TODO: handle
        return 1;
    }

    struct node* false_side = NULL;

    advance();
    if (tokens[*current_index].token_code == TOK_ELS) {

        advance();
        if (tokens[*current_index].token_code != TOK_LBR) {
            // TODO: handle
            return 1;
        }

        advance();
        int res = parse_statements(current_index, tokens, n_tokens, &false_side);
        if (res != 0) {
            return res;
        }

        if (tokens[*current_index].token_code != TOK_RBR) {
            // TODO: handle
            return 1;
        }
        advance();
    }

    struct node* if_node = node_new(NODE_IF, NULL, value, true_side, false_side);
    if (!if_node) {
        // TODO: handle
        return 1;
    }

    *root = if_node;

    return 0;
}

static int parse_while(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    struct node* true_side = NULL;

    advance();

    struct node* value = NULL;
    int res = expression(current_index, tokens, n_tokens, &value);
    if (tokens[*current_index].token_code != TOK_LBR) {
        // TODO: handle
        return 1;
    }

    advance();
    res = parse_statements(current_index, tokens, n_tokens, &true_side);
    if (res != 0) {
        return res;
    }

    if (tokens[*current_index].token_code != TOK_RBR) {
        // TODO: handle
        return 1;
    }

    advance();
    struct node* while_node = node_new(NODE_WHILE, NULL, value, true_side, NULL);
    if (!while_node) {
        // TODO: handle
        return 1;
    }

    *root = while_node;

    return 0;
}

static int parse_assignment(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    const struct token_entry* current_token = get_token();
    advance();


    if (tokens[*current_index].token_code != TOK_EQL) {
        // TODO: handle
        return 1;
    }

    struct node* var_node = node_new(NODE_VAR, current_token, NULL, NULL, NULL);
    if (!var_node) {
        // TODO: handle
        return 1;
    }

    advance();
    struct node* assignment_value = NULL;
    int res = expression(current_index, tokens, n_tokens, &assignment_value)    ;
    if (res != 0) {
        // TODO: handle
        return 1;
    }

    struct node* assignment_node = node_new(NODE_ASSIGN, NULL, var_node, assignment_value, NULL);
    if (!assignment_node) {
        // TODO: handle
        return 1;
    }

    *root = assignment_node;
    return 0;
}

static int parse_function_call(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    const struct token_entry* current_token = get_token();
    advance();

    struct node* function_value = NULL;

    if (tokens[*current_index].token_code != TOK_LPR) {
        // TODO: handle
        return 1;
    }

    advance();
    int res = expression(current_index, tokens, n_tokens, &function_value);
    if (res != 0) {
        // TODO: handle
        return 1;
    }

    if (tokens[*current_index].token_code != TOK_RPR) {
        // TODO: handle
        return 1;
    }
    advance();

    struct node* function_node = node_new(NODE_FUNCTION_CALL, current_token, function_value, NULL, NULL);
    if (!function_node) {
        // TODO: handle
        return 1;
    }

    *root = function_node;

    return 0;
}

static int parse_statement(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root) {
    int current_token_code = tokens[*current_index].token_code;

    if (current_token_code == TOK_EOF) {
        return 1;
    }

    if (current_token_code == TOK_IF) {
        return parse_if(current_index, tokens, n_tokens, root);
    }

    if (current_token_code == TOK_WHL) {
        return parse_while(current_index, tokens, n_tokens, root);
    }

    if (current_token_code == TOK_IDN) {
        if (tokens[(*current_index) + 1].token_code == TOK_EQL) {
            return parse_assignment(current_index, tokens, n_tokens, root);

        } else if (tokens[(*current_index) + 1].token_code == TOK_LPR) {
            return parse_function_call(current_index, tokens, n_tokens, root);
        }
    }

    if (current_token_code == TOK_LPR || current_token_code == TOK_INT || current_token_code == TOK_IDN) {
        return expression(current_index, tokens, n_tokens, root);
    }

    return 1;
}

static int parse_statements(uint32_t* current_index, const struct token_entry* tokens, uint32_t n_tokens, struct node** root ) {
    struct node* first_statement = node_new(NODE_STATEMENT, NULL, NULL, NULL, NULL);
    if (!first_statement) {
        //TODO: handle
        return 1;
    }

    struct node* statement = first_statement;

    while (parse_statement(current_index, tokens, n_tokens, &statement->value) == 0){
        struct node* new_statement = node_new(NODE_STATEMENT, NULL, NULL, NULL, NULL);
        if (!new_statement) {
            //TODO: handle
            return 1;
        }

        statement->left = new_statement;
        statement = new_statement;
    }

    *root = first_statement;

    return 0;
}

int parse(const struct token_entry* tokens, uint32_t n_tokens, struct node** root ) {
    uint32_t current_index = 0;
    return parse_statements(&current_index, tokens, n_tokens, root);
}
