#include <stdarg.h>
#include "../include/parser.h"
#include "../include/symbol_table.h"

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
        Token *first = *(Token **) cyclic_queue_offset(&parser->tokens, 0);
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
        parser->error = !parser->is_tracking;
        parser_unget_token(parser);
        return NULL;
    }

    return curr;
}

static ParserState *parser_state(Parser *parser)
{
    ParserState *result = malloc(sizeof *result);
    result->start_index = parser->curr_token;
    result->is_first = !parser->is_tracking;
    parser->is_tracking = true;
    return result;
}

static void parser_set_state(Parser *parser, ParserState *state)
{
    parser->curr_token = state->start_index;
}

static void parser_drop_state(Parser *parser, ParserState *state)
{
    if (!state->is_first) {
        free(state);
        return;
    }

    while (parser->curr_token > PARSER_LOOK_AHEAD) {
        Token **first = cyclic_queue_offset(&parser->tokens, 0);
        token_free(*first);
        cyclic_queue_dequeue(&parser->tokens, NULL); 
        parser->curr_token--;
    }

    parser->is_tracking = false;
    free(state);
}

static TokenType parser_panic(Parser *parser, size_t types_count, ...)
{
    va_list args;
    TokenType types[types_count];

    va_start(args, types_count);

    for (size_t i = 0; i < types_count; i++) 
        types[i] = va_arg(args, TokenType);

    va_end(args);

    while (true) {
        Token *curr = parser_get_token(parser);

        for (size_t i = 0; i < types_count; i++){
            if (curr->type == types[i])
                return types[i];
        }

        if (curr->type == TT_EOF)
            return TT_EOF;
    }
}

Expr *parser_id(Parser *parser)
{
    Token *curr = parser_expect(parser, TT_NA);
    if (curr == NULL)
        return NULL;

    bool quit = false;
    Expr *e = expr_na(curr);
    while (!quit) {
        curr = parser_get_token(parser);

        switch (curr->type) {

        case TT_LEFT_BRACKET:
            {
                Expr *index = parser_e(parser);
                if (index == NULL) {
                    expr_free(e);
                    return NULL;
                }

                Token *r_bracket = parser_expect(parser, TT_RIGHT_BRACKET);
                if (r_bracket == NULL) {
                    expr_free(e);
                    return NULL;
                }

                e = expr_arr_access(e, index, r_bracket->loc.column_end);
            }
            break;

        case TT_AT:
        case TT_AND:
            e = expr_unary(curr->type, e, curr->loc.column_end, false);
            break;

        case TT_DOT:
            {
                Token *na = parser_expect(parser, TT_NA);
                if (na == NULL) {
                    expr_free(e);
                    return NULL; 
                }

                e = expr_access(na, e);
            }
            break;

        default:
            quit = true;
            break;
        }

    }
    parser_unget_token(parser);

    return e;
}

Expr *parser_f(Parser *parser)
{
    Token *curr = parser_get_token(parser);

    switch (curr->type) {

    case TT_MINUS:
        {
            size_t column = curr->loc.column_start;
            Expr *f = parser_f(parser);
            if (f == NULL)
                return NULL;

            return expr_unary(TT_MINUS, f, column, true);
        }

    case TT_LEFT_PAREN:
        {
            Location left_loc = curr->loc;
            Expr *result = parser_e(parser);
            if (result == NULL)
                return NULL;

            Token *r = parser_expect(parser, TT_RIGHT_PAREN);
            if (r == NULL) {
                expr_free(result);
                parser_log_info(parser,
                                &left_loc,
                                "right prarenphesis is here:");
                return NULL;
            }

            result->loc.column_start = left_loc.column_start;
            result->loc.column_end = r->loc.column_end;
            return result;
        }

    case TT_C:
        return expr_c(curr);

    case TT_NA:
        parser_unget_token(parser);
        return parser_id(parser);

    default:
        parser_unget_token(parser);
        parser_log_error(parser, &curr->loc,
                         "expected factor instead of \"%s\".",
                         token_to_string(curr));
        return NULL;
    }
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
        if (f == NULL) {
            expr_free(e);
            return NULL;
        }

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
    while ((curr->type == TT_PLUS ||
            curr->type == TT_MINUS)) {
        TokenType type = curr->type;

        Expr *t = parser_t(parser);
        if (t == NULL) {
            expr_free(e);
            return NULL;
        }

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
        return expr_bc(token);

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
        expr_free(left_e);
        parser_unget_token(parser);
        parser_log_error(parser,
                         &op->loc,
                         "expected logical comparison "
                         "operand after expression."); 
        return NULL;
    }

    TokenType type = op->type;

    Expr *right_e = parser_e(parser);
    if (right_e == NULL) {
        expr_free(left_e);
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
            parser_unget_token(parser);
            ParserState *state = parser_state(parser);

            Expr *e = parser_e(parser);
            if (e != NULL) {
                parser_set_state(parser, state);
                parser_drop_state(parser, state);
                return parser_atom(parser);
            }

            parser_set_state(parser, state);
            parser_drop_state(parser, state);

            curr = parser_get_token(parser);
            Location left_loc = curr->loc;

            Expr *be = parser_be(parser);
            if (be == NULL) {
                expr_free(e);
                return NULL;
            }

            if (parser_expect(parser, TT_RIGHT_PAREN) == NULL) {
                expr_free(e);
                expr_free(be);
                parser_log_info(parser,
                                &left_loc,
                                "right prarenphesis is here:");
                return NULL;
            }

            be->loc.column_start = left_loc.column_start;
            be->loc.column_end = curr->loc.column_start;
            return be;
        }

    case TT_NA:
        {
            parser_unget_token(parser);
            ParserState *state = parser_state(parser);

            Expr *id = parser_id(parser);
            switch (parser_get_token(parser)->type) {
            case TT_PLUS:
            case TT_MINUS:
            case TT_STAR:
            case TT_SLASH:
            case TT_GREATER:
            case TT_GREATER_EQUALS:
            case TT_LESS:
            case TT_LESS_EQUALS:
            case TT_LOGICAL_EQUALS:
            case TT_NOT_EQUALS:
                parser_set_state(parser, state);
                parser_drop_state(parser, state);
                expr_free(id);
                return parser_atom(parser);

            default:
                parser_drop_state(parser, state);
                parser_unget_token(parser);
                return id;
            }
        }

    default:
        parser_unget_token(parser);
        return parser_atom(parser);
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
        if (f == NULL) {
            expr_free(e);
            return NULL;
        }

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
        if (t == NULL) {
            expr_free(e);
            return NULL;
        }

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
        return expr_cc(curr);

    parser_unget_token(parser);
    ParserState *state = parser_state(parser);
    Expr *result = parser_e(parser);
    if (result != NULL) {
        switch (parser_get_token(parser)->type) {
        case TT_GREATER:
        case TT_GREATER_EQUALS:
        case TT_LESS:
        case TT_LESS_EQUALS:
        case TT_LOGICAL_EQUALS:
        case TT_NOT_EQUALS:
            parser_set_state(parser, state);
            parser_drop_state(parser, state);
            expr_free(result);
            return parser_be(parser);

        default:
            parser_drop_state(parser, state);
            parser_unget_token(parser);
            return result;
        }
    }

    parser_set_state(parser, state);
    parser_drop_state(parser, state);

    return parser_be(parser);
}

static ArrayList *parser_args(Parser *parser)
{
    ArrayList *result = malloc(sizeof *result);
    array_list_create(result, sizeof(Expr *), 8);

    do {
        Expr *e = parser_cc_be_e(parser);
        if (e == NULL) {
            array_list_destroy(result, stmt_free_wrapper);
            free(result);
            return NULL;
        }

        array_list_append(result, &e);
    } while (parser_get_token(parser)->type == TT_COMMA);

    parser_unget_token(parser);

    return result;
}

static ArrayList *parser_stmts(Parser *parser)
{
    ArrayList *result = malloc(sizeof *result);
    array_list_create(result, sizeof(Expr *), 8);

    do {
        Token *t = parser_get_token(parser);
        parser_unget_token(parser);
        if (t->type == TT_RETURN)
            break;

        Stmt *s = parser_stmt(parser);
        if (s == NULL) {
            TokenType t = parser_panic(parser, 2, TT_SEMICOLON, TT_RIGHT_BRACE);
            if (t == TT_SEMICOLON)
                continue;

            parser_unget_token(parser);
            array_list_destroy(result, stmt_free_wrapper);
            free(result);
            return NULL;
        }

        array_list_append(result, &s);
    } while (parser_get_token(parser)->type == TT_SEMICOLON);

    parser_unget_token(parser);

    return result;
}

Stmt *parser_stmt(Parser *parser)
{
    Token *first = parser_get_token(parser);

    switch (first->type) {

    case TT_IF:
        {
            size_t start_column = first->loc.column_start;
            Expr *be = parser_be(parser);
            if (be == NULL)
                return NULL;

            Token *l_brace = parser_expect(parser, TT_LEFT_BRACE);
            if (l_brace == NULL) {
                expr_free(be);
                return NULL;
            }
            Location l_brace_loc = l_brace->loc;

            ArrayList *then_stmts = parser_stmts(parser);
            if (then_stmts == NULL) {
                expr_free(be);
                return NULL;
            }

            if (parser_expect(parser, TT_RIGHT_BRACE) == NULL) {
                parser_log_info(parser, &l_brace_loc, "left brace is here:");
                expr_free(be);
                array_list_destroy(then_stmts, stmt_free_wrapper);
                return NULL;
            }

            Token *else_token = parser_get_token(parser);
            if (else_token->type != TT_ELSE) {
                parser_unget_token(parser);
                return stmt_if(be, then_stmts, NULL, start_column);
            }

            Token *else_brace = parser_expect(parser, TT_LEFT_BRACE);
            if (else_brace == NULL) {
                expr_free(be);
                array_list_destroy(then_stmts, stmt_free_wrapper);
                return NULL;
            }
            Location else_brace_loc = else_brace->loc;

            ArrayList *else_stmts = parser_stmts(parser);
            if (else_stmts == NULL) {
                expr_free(be);
                array_list_destroy(then_stmts, stmt_free_wrapper);
                return NULL;
            }

            if (parser_expect(parser, TT_RIGHT_BRACE) == NULL) {
                parser_log_info(parser, &else_brace_loc, "left brace is here:");
                expr_free(be);
                array_list_destroy(then_stmts, stmt_free_wrapper);
                array_list_destroy(else_stmts, stmt_free_wrapper);
                return NULL;
            }

            return stmt_if(be, then_stmts, else_stmts, start_column);
        }

    case TT_WHILE:
        {
            size_t start_column = first->loc.column_start;
            Expr *be = parser_be(parser);
            if (be == NULL)
                return NULL;

            Token *l_brace = parser_expect(parser, TT_LEFT_BRACE);
            if (l_brace == NULL) {
                expr_free(be);
                return NULL;
            }

            Location l_brace_loc = l_brace->loc;

            ArrayList *stmts = parser_stmts(parser);
            if (stmts == NULL) {
                expr_free(be);
                return NULL;
            }

            if (parser_expect(parser, TT_RIGHT_BRACE) == NULL) {
                parser_log_info(parser, &l_brace_loc, "left brace is here:");
                expr_free(be);
                array_list_destroy(stmts, stmt_free_wrapper);
                return NULL;
            }

            return stmt_while(be, stmts, start_column);
        }

    default:
        {
            parser_unget_token(parser);
            Expr *id = parser_id(parser);
            if (id == NULL)
                return NULL;

            if (parser_expect(parser, TT_EQUALS) == NULL) {
                expr_free(id);
                return NULL;
            }

            Token *next = parser_get_token(parser);
            if (next->type == TT_NEW) {
                Token *na = parser_expect(parser, TT_NA);
                if (na == NULL) {
                    expr_free(id);
                    return NULL;
                }

                Token *star = parser_expect(parser, TT_STAR);
                if (star == NULL) {
                    expr_free(id);
                    return NULL;
                }

                return stmt_new(id, na, star->loc.column_end);
            }

            Token *paren = parser_get_token(parser);
            if (next->type == TT_NA && paren->type == TT_LEFT_PAREN) {
                Token *right_paren = parser_get_token(parser);
                if (right_paren->type == TT_RIGHT_PAREN) 
                    return stmt_funcall(id, next, NULL, right_paren->loc.column_end);

                parser_unget_token(parser);

                ArrayList *args = parser_args(parser);
                if (args == NULL) {
                    expr_free(id);
                    return NULL;
                }

                right_paren = parser_expect(parser, TT_RIGHT_PAREN);
                if (right_paren == NULL) {
                    expr_free(id);
                    return NULL;
                }

                return stmt_funcall(id, next, args, right_paren->loc.column_end);
            }

            parser_unget_token(parser);
            parser_unget_token(parser);

            Expr *right = parser_cc_be_e(parser);
            if (right == NULL) {
                expr_free(id);
                return NULL;
            }

            return stmt_assign(id, right);
        }
    }
}

Type *parser_ty(Parser *parser, bool should_exist)
{
    Token *curr = parser_get_token(parser);

    switch (curr->type) {
    case TT_INT:
    case TT_UINT:
    case TT_CHAR:
    case TT_BOOL:
    case TT_NA:
        {
            Type *type = type_get(curr->lexeme);
            if (type != NULL) 
                return type;

            if (should_exist) {
                parser_log_error(parser, &curr->loc, "unkown type: %s", 
                                 token_to_string(curr));
                return NULL;
            }

            return type_add(str_dup(curr->lexeme));
        }

    default:
        parser_log_error(parser, &curr->loc, "expected type but got: %s", 
                         token_to_string(curr));
        return NULL;
    }
}

static Field *parser_fields(Parser *parser, size_t *fields_count)
{
    *fields_count = 0;
    size_t allocated_fields = 4;
    Field *fields = malloc(allocated_fields * sizeof *fields);

    Token *semicolon = NULL;
    do {
        Type *type = parser_ty(parser, true);
        if (type == NULL) {
            free(fields);
            return NULL;
        }

        Token *name = parser_expect(parser, TT_NA);
        if (name == NULL) {
            free(fields);
            return NULL;
        }

        fields[*fields_count].type = type;
        fields[*fields_count].name = str_dup(name->lexeme);

        *fields_count += 1; 
        if (*fields_count >= allocated_fields) {
            allocated_fields *= 2;
            Field *tmp = realloc(fields, allocated_fields * sizeof *tmp);
            if (tmp == NULL)
                return NULL;
            fields = tmp;
        }
        semicolon = parser_get_token(parser);
    } while(semicolon->type == TT_SEMICOLON);
    parser_unget_token(parser);

    return fields;
}

bool parser_tyd(Parser *parser)
{
    Token *struct_token = parser_get_token(parser);
    if (struct_token->type == TT_STRUCT) {
        if (parser_expect(parser, TT_LEFT_BRACE) == NULL)
            return false;

        size_t fields_count = 0;
        Field *fields = parser_fields(parser, &fields_count);
        if (fields == NULL)
            return false;

        if (parser_expect(parser, TT_RIGHT_BRACE) == NULL)
            return false;

        Token *name = parser_expect(parser, TT_NA);
        if (name == NULL) 
            return false;

        if (type_struct(str_dup(name->lexeme), fields, fields_count) == NULL) {
            parser_log_error(parser, &name->loc, 
                             "type with name %s already exists", 
                             name->lexeme); 
            return false;    
        }

        return true;
    }

    parser_unget_token(parser);

    Type *type = parser_ty(parser, false);
    if (type == NULL)
        return false;

    Token *op = parser_get_token(parser);
    switch (op->type) {

    case TT_LEFT_BRACKET:
        {
            Token *dig = parser_expect(parser, TT_C);
            if (dig == NULL)
                return false;

            if (dig->is_null) {
                parser_log_error(parser, &dig->loc, 
                                 "expected digit but got: %s", 
                                 token_to_string(dig));
                return false;
            }

            size_t elements = dig->value_as.integer;

            if (parser_expect(parser, TT_RIGHT_BRACKET) == NULL)
                return false;

            Token *name = parser_expect(parser, TT_NA);
            if (name == NULL) 
                return false;

            if (!type->is_defined) {
                parser_log_error(parser, &name->loc, 
                                 "only pointers can predifined types");
                return false;
            }

            if (type_array(str_dup(name->lexeme), type, elements) == NULL) {
                parser_log_error(parser, &name->loc, 
                                 "type with name %s already exists", 
                                 name->lexeme); 
                return false;    
            }

            return true;
        }

    case TT_STAR:
        {
            Token *name = parser_expect(parser, TT_NA);
            if (name == NULL) 
                return false;

            if (type_pointer(str_dup(name->lexeme), type) == NULL) {
                parser_log_error(parser, &name->loc, 
                                 "type with name %s already exists", 
                                 name->lexeme); 
                return false;    
            }

            return true;
        }

    default:
        parser_log_error(parser, &op->loc, "unexpected %s", 
                         token_to_string(op)); 
        return false;
    }
}

bool parser_tyds(Parser *parser)
{
    bool first = true;

    Token *tydef = parser_get_token(parser);
    while (tydef->type == TT_TYPEDEF) {
        if (!parser_tyd(parser))
            return false;

        Token *semicolon = parser_get_token(parser);
        if (semicolon->type != TT_SEMICOLON) {
            parser_unget_token(parser);
            return true;
        }

        tydef = parser_get_token(parser);
        first = false;
    }

    if (!first)
        parser_unget_token(parser); 

    parser_unget_token(parser);
    return true;
}

bool parser_global_vad(Parser *parser)
{
    Type *type = parser_ty(parser, true);
    if (type == NULL) 
        return false;

    Token *name = parser_expect(parser, TT_NA);
    if (name == NULL)
        return false;


    Symbol *sym = symtable_get(global_syms, name->lexeme);
    if (sym != NULL) {
        parser_log_error(parser, 
                         &name->loc, 
                         "variable with name %s already exists", 
                         name->lexeme);
        parser_log_info(parser, 
                        &sym->loc, 
                        "%s previously defined here",
                        name->lexeme);
        return false;
    }

    sym = symbol_create(str_dup(name->lexeme), type, SS_GLOBAL, &name->loc);
    symtable_add(global_syms, sym);
    return true;
}

static bool parser_is_type(Token *na)
{
    switch (na->type) {
        case TT_BOOL:
        case TT_INT:
        case TT_CHAR:
        case TT_UINT:
            return true;

        case TT_NA:
            return type_get(na->lexeme) != NULL;

        default:
            return false;
    }
}

static int parser_local_vads(Parser *parser, SymTable *local)
{
    int count = 0;
    do {
        Token *type = parser_get_token(parser);
        Token *name = parser_get_token(parser);

        if (type->type == TT_RETURN || 
            type->type == TT_IF || 
            type->type == TT_WHILE || 
            name->type != TT_NA) {
            parser_unget_token(parser);
            parser_unget_token(parser);
            break;
        }

        if (!parser_is_type(type)) {
            parser_log_error(parser,
                             &type->loc,
                             "unknown type: %s",
                             token_to_string(type));
            return -1;
        }

        count += 1;
        Symbol *new = symbol_create(str_dup(name->lexeme), 
                                    type_get(type->lexeme),
                                    SS_LOCAL,
                                    &name->loc); 
        symtable_add(local, new);

    } while (parser_get_token(parser)->type == TT_SEMICOLON);
    
    if (count > 0)
        parser_unget_token(parser);
    
    return count;
}

Function *parser_fud(Parser *parser)
{
    Type *return_type = parser_ty(parser, true);
    if (return_type == NULL)
        return NULL;

    Token *name_token = parser_expect(parser, TT_NA);
    if (name_token == NULL)
        return NULL;

    char *name = str_dup(name_token->lexeme);

    if (parser_expect(parser, TT_LEFT_PAREN) == NULL) 
        goto clean_name;

    SymTable *local = symtable_create(global_syms);

    Token *token = parser_get_token(parser);
    switch (token->type) {
    case TT_RIGHT_PAREN:
        break;

    default:
        break;
    } 

    if (parser_expect(parser, TT_LEFT_BRACE) == NULL) 
        goto clean_symtable;

    int local_vads_result = parser_local_vads(parser, local);
    if (local_vads_result == -1 ||
        (local_vads_result > 0 && parser_expect(parser, TT_SEMICOLON) == NULL))
        goto clean_symtable;

    Token *t = parser_get_token(parser);
    parser_unget_token(parser);

    ArrayList *stmts = NULL;
    if (t->type != TT_RETURN) {
        stmts = parser_stmts(parser);
        if (parser_expect(parser, TT_SEMICOLON) == NULL) 
            goto clean_symtable;
    }

    t = parser_expect(parser, TT_RETURN);
    if (t == NULL) 
        goto clean_stmts;

    size_t start_column = t->loc.column_start;

    Expr *e = parser_cc_be_e(parser);
    if (e == NULL)
        goto clean_stmts;

    Stmt *return_stmt = stmt_return(e, start_column);

    if (parser_expect(parser, TT_RIGHT_BRACE) == NULL)
        goto clean_all;

    return function_create(name, local, stmts, return_stmt);

clean_all:
    stmt_free(return_stmt);
    expr_free(e);

clean_stmts:
    if (stmts != NULL)
        array_list_destroy(stmts, stmt_free_wrapper);

clean_symtable:
    symtable_destroy(local);

clean_name:
    free(name);

    return NULL;
}
