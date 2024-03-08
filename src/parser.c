#include "../include/parser.h"

#define parser_log_info(parser, ...)                    \
    if (!parser->is_tracking)                           \
        log_print_with_location(LOG_INFO, __VA_ARGS__)

#define parser_log_error(parser, ...)                   \
    if (!parser->is_tracking)                           \
        log_print_with_location(LOG_ERROR, __VA_ARGS__)        

Parser *parser_alloc(Lexer *lexer)
{
    Parser *result = malloc(sizeof *result);
    result->lexer = lexer;
    result->error = false;
    result->is_tracking = false;
    result->curr_token = 0;
    cyclic_queue_create(&result->tokens, sizeof(Token *), 8);

    for (size_t i = 0; i < PARSER_LOOK_AHEAD; i++) {
        Token *new = lexer_next(lexer);
        cyclic_queue_enqueue(&result->tokens, &new);
    }

    return result;
}

void parser_free(Parser *parser)
{
    lexer_free(parser->lexer);

    for (size_t i = 0; i < parser->tokens.size; i++) {
        Token **curr = cyclic_queue_offset(&parser->tokens, i);
        if (!(*curr)->is_owned)
            token_free(*curr);
    }

    cyclic_queue_destroy(&parser->tokens);

    free(parser);
}

static Token *parser_get_token(Parser *parser)
{
    if (parser->curr_token == parser->tokens.size) {
        Token *new = lexer_next(parser->lexer);
        cyclic_queue_enqueue(&parser->tokens, &new);
    }

    Token *result = *(Token **) cyclic_queue_offset(&parser->tokens,
                                                    parser->curr_token);

    parser->curr_token++;

    if (!parser->is_tracking && parser->curr_token > PARSER_LOOK_AHEAD) {
        Token *first = *(Token **) cyclic_queue_offset(&parser->tokens,
                                                       0);
        if (!first->is_owned)
            token_free(first);

        cyclic_queue_dequeue(&parser->tokens, NULL);

        parser->curr_token--;
    }

    return result;
}

static void parser_unget_token(Parser *parser)
{
    if (parser->curr_token == 0) {
        log_error("Unget called on empty queue");
        return;
    }

    parser->curr_token--;
}

static Token *parser_expect(Parser *parser, TokenType type)
{
    Token *curr = parser_get_token(parser);
     
    if (curr->type != type) {
        parser_log_error(parser, &curr->loc,
                         "expected \"%s\", but got \"%s\".",
                         token_type_strings[type],
                         token_to_string(curr));
        parser->error = true;
        return NULL;
    }

    return curr;
}

static size_t parser_state(Parser *parser)
{
    parser->is_tracking = true;
    return parser->curr_token;
}

static void parser_set_state(Parser *parser, size_t state)
{
    parser->curr_token = state;
}

static void parser_drop_state(Parser *parser, size_t state)
{
    if (state > PARSER_LOOK_AHEAD)
        return;

    while (parser->curr_token > PARSER_LOOK_AHEAD) {
        Token **first = cyclic_queue_offset(&parser->tokens, 0);

        if (!(*first)->is_owned)
            token_free(*first);

        cyclic_queue_dequeue(&parser->tokens, NULL); 

        parser->curr_token--;
    }

    parser->is_tracking = false;
}


Expr *parser_id(Parser *parser)
{
    Token *curr = parser_expect(parser, TT_Na);
    if (curr == NULL)
        return NULL;

    Expr *e = expr_token(curr);

    while (true) {
        curr = parser_get_token(parser);

        switch (curr->type) {

        case TT_LEFT_BRACKET:
            {
                Expr *index = parser_e(parser);

                if (index == NULL)
                    return NULL;

                Token *r_bracket = parser_expect(parser, TT_RIGHT_BRACKET);

                if (r_bracket == NULL)
                    return NULL;

                e = expr_arr_access(e, index, r_bracket->loc.column_end);
            }
            break;

        case TT_AT:
        case TT_AND:
            e = expr_unary(curr->type, e, curr->loc.column_end, false);
            break;

        case TT_DOT:
            {
                Token *na = parser_expect(parser, TT_Na);

                if (na == NULL) {
                    expr_free(e, !parser->is_tracking);

                    return NULL; // TODO: implement error 
                }

                e = expr_access(na, e);
            }
            break;

        default:
            goto while_end;
        }
            
    }

 while_end:
    parser_unget_token(parser);

    return e;
}

Expr *parser_f(Parser *parser)
{
    Token *curr = parser_get_token(parser);

    Expr *result;

    switch (curr->type) {
    case TT_MINUS:
        {
            size_t column = curr->loc.column_start;
            Expr *f = parser_f(parser);
            if (f == NULL)
                return NULL;

            result = expr_unary(TT_MINUS, f, column, true);
        }
        break;

    case TT_LEFT_PAREN:
        {
            Location left_loc = curr->loc;
            result = parser_e(parser);
            if (result == NULL)
                return NULL;

            Token *r = parser_expect(parser, TT_RIGHT_PAREN);
            if (r == NULL) {
                // TODO: ERROR
                expr_free(result, !parser->is_tracking);
                parser_log_info(parser,
                                &left_loc,
                                "right prarenphesis is here:");
                return NULL;
            }

            result->loc.column_start = left_loc.column_start;
            result->loc.column_end = r->loc.column_end;
        }
        break;

    case TT_C:
        result = expr_token(curr);
        break;

    case TT_Na:
        {
            parser_unget_token(parser);
            result = parser_id(parser);
        }
        break;

    default:

        parser_log_error(parser, &curr->loc,
                         "unexpected \"%s\".",
                         token_to_string(curr));
        return NULL;
    }

    return result;
}

Expr *parser_t(Parser *parser)
{
    Expr *e = parser_f(parser);
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token(parser);

    while ((curr->type == TT_STAR ||
            curr->type == TT_SLASH)) {
        TokenType type = curr->type;

        Expr *f = parser_f(parser);

        if (f == NULL) 
            return NULL;

        e = expr_binary(type, e, f);

        curr = parser_get_token(parser);
    }
    parser_unget_token(parser);
    
    return e;
}

Expr *parser_e(Parser *parser)
{
    Expr *e = parser_t(parser);
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token(parser);

    while ((curr->type ==  TT_PLUS ||
            curr->type == TT_MINUS)) {

        TokenType type = curr->type;

        Expr *t = parser_t(parser);

        if (t == NULL)
            return NULL;

        e = expr_binary(type, e, t);

        curr = parser_get_token(parser);
    }

    parser_unget_token(parser);
    
    return e;
}

Expr *parser_atom(Parser *parser)
{
    Token *token = parser_get_token(parser);

    if (token->type == TT_BC) 
        return expr_token(token);

    parser_unget_token(parser);

    Expr *left_e = parser_e(parser);
    if (left_e == NULL)
        return NULL;

    Token *op = parser_get_token(parser);

    switch (op->type) {
    case TT_GREATER:
    case TT_GREATER_EQUALS:
    case TT_LESS:
    case TT_LESS_EQUALS:
    case TT_LOGICAL_EQUALS:
    case TT_NOT_EQUALS:
        break;
        
    default:
        parser_log_error(parser,
                         &left_e->loc,
                         "expected logical comparison operand after expression."); 
        expr_free(left_e, !parser->is_tracking);
        return NULL;
    }

    TokenType type = op->type;
    Location loc = op->loc;

    Expr *right_e = parser_e(parser);
    if (right_e == NULL) {
        expr_free(left_e, !parser->is_tracking);
        parser_log_info(parser, &loc, "operator expects right operand.");
        return NULL;
    }

    return expr_binary(type, left_e, right_e);
}

Expr *parser_bf(Parser *parser)
{
    Token *curr = parser_get_token(parser);

    switch (curr->type) {

    case TT_NOT:
        {
            size_t column = curr->loc.column_start;
            Expr *bf = parser_bf(parser);
            if (bf == NULL)
                return NULL;

            return expr_unary(TT_NOT, bf, column, true);
        }

    case TT_LEFT_PAREN:
        {
            size_t state = parser_state(parser);
            Location left_loc = curr->loc;
            Expr *bf = parser_bf(parser);
            if (bf != NULL) {
                parser_drop_state(parser, state);

                curr = parser_get_token(parser);
                if (curr->type != TT_RIGHT_PAREN) {
                    //TODO: Add error
                    expr_free(bf, !parser->is_tracking);
                    parser_log_info(parser,
                                    &left_loc,
                                    "right prarenphesis is here:");
                    return NULL;
                }

                bf->loc.column_start = left_loc.column_start;
                bf->loc.column_end = curr->loc.column_start;

                return bf;
            }

            parser_set_state(parser, state);
            parser_unget_token(parser);
            parser_drop_state(parser, state);

            return parser_atom(parser);
        }

    default:
        {
            parser_unget_token(parser);

            Expr *e = parser_atom(parser);

            return e;
        }
    }
}

Expr *parser_bt(Parser *parser)
{
    Expr *e = parser_bf(parser);
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token(parser);

    while (curr->type == TT_LOGICAL_AND) {
        TokenType type = curr->type;

        Expr *f = parser_bf(parser);
        if (f == NULL) 
            return NULL;

        e = expr_binary(type, e, f);

        curr = parser_get_token(parser);
    }

    parser_unget_token(parser);
    
    return e;
}

Expr *parser_be(Parser *parser)
{
    Expr *e = parser_bt(parser);
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token(parser);

    while (curr->type == TT_LOGICAL_OR) {

        TokenType type = curr->type;

        Expr *t = parser_bt(parser);

        if (t == NULL)
            return NULL;

        e = expr_binary(type, e, t);

        curr = parser_get_token(parser);
    }

    parser_unget_token(parser);
    
    return e;
}

static Expr *parser_cc_be_e(Parser *parser)
{
    Token *curr = parser_get_token(parser);
    if (curr->type == TT_CC)
        return expr_token(curr);

    parser_unget_token(parser);
    size_t state = parser_state(parser);
    Expr *result = parser_be(parser);
    if (result != NULL) {
        parser_drop_state(parser, state);
        return result;
    }

    parser_set_state(parser, state);
    parser_drop_state(parser, state);

    return parser_e(parser);
}

Stmt *parser_assignment(Parser *parser)
{
    Expr *left = parser_id(parser);
    if (left == NULL)
        return NULL;

    if (parser_expect(parser, TT_EQUALS) == NULL)
        return NULL;

    Expr *right = parser_cc_be_e(parser);
    if (right == NULL) 
        return NULL;

    return stmt_assign(left, right);
}


static ArrayList *parser_args(Parser *parser)
{
    ArrayList *result = malloc(sizeof *result);
    array_list_create(result, sizeof(Expr *), 8);

    Expr *e = parser_cc_be_e(parser);
    if (e == NULL) {
        array_list_destroy(result);
        free(result);
        return NULL;
    }

    array_list_append(result, &e);
    
    while (parser_get_token(parser)->type == TT_COMMA) {
        e = parser_cc_be_e(parser);
        if (e == NULL) {
            for (size_t i = 0; i < result->size; i++) 
                expr_free(*(Expr **)array_list_offset(result, i),
                          !parser->is_tracking);

            array_list_destroy(result);
            free(result);
            return NULL;
        }

        array_list_append(result, &e);
    }

    parser_unget_token(parser);
    
    return result;
}

Stmt *parser_funcall(Parser *parser)
{
    Expr *left = parser_id(parser);
    if (left == NULL)
        return NULL;

    if (parser_expect(parser, TT_EQUALS) == NULL)
        return NULL;

    Token *na = parser_expect(parser, TT_Na);
    if (na == NULL)
        return NULL;

    if (parser_expect(parser, TT_LEFT_PAREN) == NULL)
        return NULL;

    Token *right_paren = parser_get_token(parser);

    if (right_paren->type == TT_RIGHT_PAREN) 
        return stmt_funcall(left, na, NULL, right_paren->loc.column_end);

    parser_unget_token(parser);

    ArrayList *args = parser_args(parser);
    if (args == NULL)
        return NULL;

    right_paren = parser_expect(parser, TT_RIGHT_PAREN);
    if (right_paren == NULL)
        return NULL;

    return stmt_funcall(left, na, args, right_paren->loc.column_end);
}

Stmt *parser_new(Parser *parser)
{
    Expr *left = parser_id(parser);
    if (left == NULL)
        return NULL;

    if (parser_expect(parser, TT_EQUALS) == NULL)
        return NULL;

    if (parser_expect(parser, TT_NEW) == NULL)
        return NULL;

    Token *na = parser_expect(parser, TT_Na);
    if (na == NULL)
        return NULL;

    Token *star = parser_expect(parser, TT_STAR);
    if (star == NULL)
        return NULL;

    return stmt_new(left, na, star->loc.column_end);
}
