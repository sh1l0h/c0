#include <stdarg.h>
#include "../include/parser.h"
#include "../include/symbol_table.h"

#define parser_log_info(...)                            \
    if (!parser->is_tracking)                           \
        log_print_with_location(LOG_INFO, __VA_ARGS__)

#define parser_log_error(...)                           \
    if (!parser->is_tracking)                           \
        log_print_with_location(LOG_ERROR, __VA_ARGS__)        

Parser *parser = NULL;

void parser_init()
{
    parser = malloc(sizeof *parser);
    parser->lexer = lexer;
    parser->error = false;
    parser->is_tracking = false;
    parser->oldest_state = 0;
    parser->curr_token = 0;
    cyclic_queue_create(&parser->tokens, sizeof(Token *), 8);

    for (size_t i = 0; i < PARSER_LOOK_AHEAD; i++) {
        Token *new = lexer_next(lexer);
        cyclic_queue_enqueue(&parser->tokens, &new);
    }
}

void parser_deinit()
{
    for (size_t i = 0; i < parser->tokens.size; i++) {
        Token **curr = cyclic_queue_offset(&parser->tokens, i);
        token_destroy(*curr);
    }

    cyclic_queue_destroy(&parser->tokens);

    free(parser);
}

static Token *parser_get_token()
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
        token_destroy(first);
        cyclic_queue_dequeue(&parser->tokens, NULL);
        parser->curr_token--;
    }

    return result;
}

static void parser_unget_token()
{
    if (parser->curr_token == 0) {
        log_error("Unget called on empty queue");
        return;
    }

    parser->curr_token--;
}

static Token *parser_expect(TokenType type)
{
    Token *curr = parser_get_token();

    if (!token_is_type(curr, type)) {
        parser_log_error(&curr->loc,
                         "expected \"%s\", but got \"%s\".",
                         token_strings[type],
                         curr->lexeme);
        parser->error = !parser->is_tracking;
        parser_unget_token();
        return NULL;
    }

    return curr;
}

static size_t parser_state()
{
    if (!parser->is_tracking) {
        parser->oldest_state = parser->curr_token;
        parser->is_tracking = true;
    }

    return parser->curr_token;
}

static void parser_set_state(size_t state)
{
    parser->curr_token = state;
}

static void parser_drop_state(size_t state)
{
    if (!parser->is_tracking || state != parser->oldest_state)
        return;

    while (parser->curr_token > PARSER_LOOK_AHEAD) {
        Token **first = cyclic_queue_offset(&parser->tokens, 0);
        token_destroy(*first);
        cyclic_queue_dequeue(&parser->tokens, NULL); 
        parser->curr_token--;
    }

    parser->is_tracking = false;
}

static TokenType parser_panic(size_t types_count, ...)
{
    va_list args;
    TokenType types[types_count];

    va_start(args, types_count);

    for (size_t i = 0; i < types_count; i++) 
        types[i] = va_arg(args, TokenType);

    va_end(args);

    while (true) {
        Token *curr = parser_get_token();

        for (size_t i = 0; i < types_count; i++){
            if (token_is_type(curr, types[i]))
                return types[i];
        }

        if (curr->type == TT_EOF)
            return TT_EOF;
    }
}

Expr *parser_id()
{
    Token *curr = parser_expect(TT_NA);
    if (curr == NULL)
        return NULL;

    bool quit = false;
    Expr *e = expr_na(curr);
    while (!quit) {
        curr = parser_get_token();
        switch (curr->type) {
        case TT_LEFT_BRACKET:
            {
                Expr *index = parser_e();
                if (index == NULL)
                    goto clean_e;

                Token *r_bracket = parser_expect(TT_RIGHT_BRACKET);
                if (r_bracket == NULL)
                    goto clean_e;

                e = expr_arr_access(e, index, r_bracket->loc.column_end);
            }
            break;

        case TT_AT:
        case TT_AND:
            e = expr_unary(curr->type, e, curr->loc.column_end, false);
            break;

        case TT_DOT:
            {
                Token *na = parser_expect(TT_NA);
                if (na == NULL)
                    goto clean_e;

                e = expr_access(na, e);
            }
            break;

        default:
            quit = true;
            break;
        }
    }
    parser_unget_token();

    return e;

clean_e:
    expr_free(e);
    return NULL; 
}

Expr *parser_f()
{
    Token *curr = parser_get_token();

    switch (curr->type) {
    case TT_MINUS:
        {
            size_t column = curr->loc.column_start;
            Expr *f = parser_f();
            if (f == NULL)
                return NULL;

            return expr_unary(TT_MINUS, f, column, true);
        }

    case TT_LEFT_PAREN:
        {
            Location left_loc = curr->loc;
            Expr *result = parser_e();
            if (result == NULL)
                return NULL;

            Token *r = parser_expect(TT_RIGHT_PAREN);
            if (r == NULL) {
                expr_free(result);
                parser_log_info(&left_loc,
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
        parser_unget_token();
        return parser_id();

    default:
        parser_unget_token();
        parser_log_error(&curr->loc,
                         "expected factor instead of \"%s\".",
                         curr->lexeme);
        return NULL;
    }
}

Expr *parser_t()
{
    Expr *e = parser_f();
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token();
    while ((token_is_type(curr, TT_STAR) ||
            token_is_type(curr, TT_SLASH))) {
        TokenType type = curr->type;

        Expr *f = parser_f();
        if (f == NULL) {
            expr_free(e);
            return NULL;
        }

        e = expr_binary(type, e, f);
        curr = parser_get_token();
    }
    parser_unget_token();

    return e;
}

Expr *parser_e()
{
    Expr *e = parser_t();
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token();
    while ((token_is_type(curr, TT_PLUS) ||
            token_is_type(curr, TT_MINUS))) {
        TokenType type = curr->type;

        Expr *t = parser_t();
        if (t == NULL) {
            expr_free(e);
            return NULL;
        }

        e = expr_binary(type, e, t);
        curr = parser_get_token();
    }
    parser_unget_token();

    return e;
}

static Expr *parser_atom(Expr *left_e)
{
    if (left_e == NULL) {
        Token *token = parser_get_token();
        if (token->type == TT_BC) 
            return expr_bc(token);

        parser_unget_token();

        left_e = parser_e();
        if (left_e == NULL)
            return NULL;
    }

    Token *op = parser_get_token();
    if ((!token_is_type(op, TT_GREATER)        &&
         !token_is_type(op, TT_GREATER_EQUALS) &&
         !token_is_type(op, TT_LESS)           &&
         !token_is_type(op, TT_LESS_EQUALS)    &&
         !token_is_type(op, TT_LOGICAL_EQUALS) &&
         !token_is_type(op, TT_NOT_EQUALS))) {

        expr_free(left_e);
        parser_unget_token();
        parser_log_error(&op->loc,
                         "expected logical comparison "
                         "operand after expression."); 
        return NULL;
    }

    TokenType type = op->type;

    Expr *right_e = parser_e();
    if (right_e == NULL) {
        expr_free(left_e);
        return NULL;
    }

    return expr_binary(type, left_e, right_e);
}

Expr *parser_bf()
{
    Token *curr = parser_get_token();

    switch (curr->type) {

    case TT_NOT:
        {
            size_t column = curr->loc.column_start;
            Expr *bf = parser_bf();
            if (bf == NULL)
                return NULL;

            return expr_unary(TT_NOT, bf, column, true);
        }

    case TT_LEFT_PAREN:
        {
            parser_unget_token();
            size_t state = parser_state();

            Expr *e = parser_e();
            if (e != NULL) {
                parser_drop_state(state);
                return parser_atom(e);
            }

            parser_set_state(state);
            parser_drop_state(state);

            curr = parser_get_token();
            Location left_loc = curr->loc;

            Expr *be = parser_be();
            if (be == NULL) 
                return NULL;

            if (parser_expect(TT_RIGHT_PAREN) == NULL) {
                expr_free(be);
                parser_log_info(&left_loc,
                                "right prarenphesis is here:");
                return NULL;
            }

            be->loc.column_start = left_loc.column_start;
            be->loc.column_end = curr->loc.column_start;
            return be;
        }

    case TT_NA:
        {
            parser_unget_token();
            size_t state = parser_state();

            Expr *id = parser_id();
            switch (parser_get_token()->type) {
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
                parser_set_state(state);
                parser_drop_state(state);
                expr_free(id);
                return parser_atom(NULL);

            default:
                parser_drop_state(state);
                parser_unget_token();
                return id;
            }
        }

    default:
        parser_unget_token();
        return parser_atom(NULL);
    }
}

Expr *parser_bt()
{
    Expr *e = parser_bf();
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token();
    while (curr->type == TT_LOGICAL_AND) {
        TokenType type = curr->type;

        Expr *f = parser_bf();
        if (f == NULL) {
            expr_free(e);
            return NULL;
        }

        e = expr_binary(type, e, f);
        curr = parser_get_token();
    }

    parser_unget_token();
    return e;
}

Expr *parser_be()
{
    Expr *e = parser_bt();
    if (e == NULL)
        return NULL;

    Token *curr = parser_get_token();
    while (curr->type == TT_LOGICAL_OR) {
        TokenType type = curr->type;

        Expr *t = parser_bt();
        if (t == NULL) {
            expr_free(e);
            return NULL;
        }

        e = expr_binary(type, e, t);
        curr = parser_get_token();
    }

    parser_unget_token();
    return e;
}

static Expr *parser_cc_be_e()
{
    Token *curr = parser_get_token();
    if (curr->type == TT_CC)
        return expr_cc(curr);

    parser_unget_token();
    size_t state = parser_state();
    Expr *result = parser_e();
    if (result != NULL) {
        switch (parser_get_token()->type) {
        case TT_GREATER:
        case TT_GREATER_EQUALS:
        case TT_LESS:
        case TT_LESS_EQUALS:
        case TT_LOGICAL_EQUALS:
        case TT_NOT_EQUALS:
            parser_set_state(state);
            parser_drop_state(state);
            expr_free(result);
            return parser_be();

        default:
            parser_drop_state(state);
            parser_unget_token();
            return result;
        }
    }

    parser_set_state(state);
    parser_drop_state(state);

    return parser_be();
}

static Expr **parser_args()
{
    size_t allocated = 8;
    size_t size = 0;
    Expr **result = malloc(allocated * sizeof *result);

    do {
        Expr *e = parser_cc_be_e();
        if (e == NULL) {
            exprs_free(result);
            return NULL;
        }

        result[size] = e;
        result[++size] = NULL;

        if (size < allocated - 1)
            continue;

        allocated *= 2;
        result = realloc(result, allocated * sizeof *result);

    } while (parser_get_token()->type == TT_COMMA);
    parser_unget_token();

    result = realloc(result, (size + 1) * sizeof *result);
    return result;
}

static Stmt **parser_stmts()
{
    size_t allocated = 8;
    size_t size = 0;
    Stmt **result = malloc(allocated * sizeof *result);

    do {
        Token *t = parser_get_token();
        parser_unget_token();
        if (t->type == TT_RETURN)
            break;

        Stmt *s = parser_stmt();
        if (s == NULL) {
            TokenType t = parser_panic(2, TT_SEMICOLON, TT_RIGHT_BRACE);
            if (t == TT_SEMICOLON)
                continue;

            parser_unget_token();
            stmts_free(result);
            return NULL;
        }

        result[size] = s;
        result[++size] = NULL;

        if (size < allocated - 1)
            continue;

        allocated *= 2;
        result = realloc(result, allocated * sizeof *result);
    } while (parser_get_token()->type == TT_SEMICOLON);
    parser_unget_token();

    result = realloc(result, (size + 1) * sizeof *result);
    return result;
}

Stmt *parser_stmt()
{
    Token *first = parser_get_token();

    switch (first->type) {

    case TT_IF:
        {
            size_t start_column = first->loc.column_start;
            Expr *be = parser_be();
            if (be == NULL)
                return NULL;

            Token *l_brace = parser_expect(TT_LEFT_BRACE);
            if (l_brace == NULL) {
                expr_free(be);
                return NULL;
            }
            Location l_brace_loc = l_brace->loc;

            Stmt **then_stmts = parser_stmts();
            if (then_stmts == NULL) {
                expr_free(be);
                return NULL;
            }

            if (parser_expect(TT_RIGHT_BRACE) == NULL) {
                parser_log_info(&l_brace_loc, "left brace is here:");
                expr_free(be);
                stmts_free(then_stmts);
                return NULL;
            }

            Token *else_token = parser_get_token();
            if (else_token->type != TT_ELSE) {
                parser_unget_token();
                return stmt_if(be, then_stmts, NULL, start_column);
            }

            Token *else_brace = parser_expect(TT_LEFT_BRACE);
            if (else_brace == NULL) {
                expr_free(be);
                stmts_free(then_stmts);
                return NULL;
            }
            Location else_brace_loc = else_brace->loc;

            Stmt **else_stmts = parser_stmts();
            if (else_stmts == NULL) {
                expr_free(be);
                stmts_free(then_stmts);
                return NULL;
            }

            if (parser_expect(TT_RIGHT_BRACE) == NULL) {
                parser_log_info(&else_brace_loc, "left brace is here:");
                expr_free(be);
                stmts_free(then_stmts);
                stmts_free(else_stmts);
                return NULL;
            }

            return stmt_if(be, then_stmts, else_stmts, start_column);
        }

    case TT_WHILE:
        {
            size_t start_column = first->loc.column_start;
            Expr *be = parser_be();
            if (be == NULL)
                return NULL;

            Token *l_brace = parser_expect(TT_LEFT_BRACE);
            if (l_brace == NULL) {
                expr_free(be);
                return NULL;
            }

            Location l_brace_loc = l_brace->loc;

            Stmt **stmts = parser_stmts();
            if (stmts == NULL) {
                expr_free(be);
                return NULL;
            }

            if (parser_expect(TT_RIGHT_BRACE) == NULL) {
                parser_log_info(&l_brace_loc, "left brace is here:");
                expr_free(be);
                stmts_free(stmts); 
                return NULL;
            }

            return stmt_while(be, stmts, start_column);
        }

    default:
        {
            parser_unget_token();
            Expr *id = parser_id();
            if (id == NULL)
                return NULL;

            if (parser_expect(TT_EQUALS) == NULL) {
                expr_free(id);
                return NULL;
            }

            Token *next = parser_get_token();
            if (next->type == TT_NEW) {
                Token *na = parser_expect(TT_NA);
                if (na == NULL) {
                    expr_free(id);
                    return NULL;
                }

                Token *star = parser_expect(TT_STAR);
                if (star == NULL) {
                    expr_free(id);
                    return NULL;
                }

                return stmt_new(id, na, star->loc.column_end);
            }

            Token *paren = parser_get_token();
            if (next->type == TT_NA && paren->type == TT_LEFT_PAREN) {
                Token *right_paren = parser_get_token();
                if (right_paren->type == TT_RIGHT_PAREN) 
                    return stmt_funcall(id, next, NULL, right_paren->loc.column_end);

                parser_unget_token();

                Expr **args = parser_args();
                if (args == NULL) {
                    expr_free(id);
                    return NULL;
                }

                right_paren = parser_expect(TT_RIGHT_PAREN);
                if (right_paren == NULL) {
                    expr_free(id);
                    exprs_free(args);
                    return NULL;
                }

                return stmt_funcall(id, next, args, right_paren->loc.column_end);
            }

            parser_unget_token();
            parser_unget_token();

            Expr *right = parser_cc_be_e();
            if (right == NULL) {
                expr_free(id);
                return NULL;
            }

            return stmt_assign(id, right);
        }
    }
}

Type *parser_ty(bool should_exist)
{
    Token *curr = parser_get_token();

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
                parser_log_error(&curr->loc, "unkown type: %s", 
                                 curr->lexeme);
                return NULL;
            }

            return type_add(curr->lexeme);
        }

    default:
        parser_log_error(&curr->loc, "expected type but got: %s", 
                         curr->lexeme);
        return NULL;
    }
}

static Field *parser_fields(size_t *fields_count)
{
    *fields_count = 0;
    size_t allocated_fields = 4;
    Field *fields = malloc(allocated_fields * sizeof *fields);

    Token *semicolon = NULL;
    do {
        Type *type = parser_ty(true);
        if (type == NULL) {
            free(fields);
            return NULL;
        }

        Token *name = parser_expect(TT_NA);
        if (name == NULL) {
            free(fields);
            return NULL;
        }

        fields[*fields_count].type = type;
        fields[*fields_count].name = name->lexeme;

        *fields_count += 1; 
        if (*fields_count >= allocated_fields) {
            allocated_fields *= 2;
            Field *tmp = realloc(fields, allocated_fields * sizeof *tmp);
            if (tmp == NULL)
                return NULL;
            fields = tmp;
        }
        semicolon = parser_get_token();
    } while(semicolon->type == TT_SEMICOLON);
    parser_unget_token();

    return fields;
}

bool parser_tyd()
{
    Token *struct_token = parser_get_token();
    if (struct_token->type == TT_STRUCT) {
        if (parser_expect(TT_LEFT_BRACE) == NULL)
            return false;

        size_t fields_count = 0;
        Field *fields = parser_fields(&fields_count);
        if (fields == NULL)
            return false;

        if (parser_expect(TT_RIGHT_BRACE) == NULL)
            return false;

        Token *name = parser_expect(TT_NA);
        if (name == NULL) 
            return false;

        if (type_struct(name->lexeme, fields, fields_count) == NULL) {
            parser_log_error(&name->loc, 
                             "type with name %s already exists", 
                             name->lexeme); 
            return false;    
        }

        return true;
    }

    parser_unget_token();

    Type *type = parser_ty(false);
    if (type == NULL)
        return false;

    Token *op = parser_get_token();
    switch (op->type) {

    case TT_LEFT_BRACKET:
        {
            Token *dig = parser_expect(TT_C);
            if (dig == NULL)
                return false;

            if (dig->is_null) {
                parser_log_error(&dig->loc, 
                                 "expected digit but got: %s", 
                                 dig->lexeme);
                return false;
            }

            size_t elements = dig->value_as.integer;

            if (parser_expect(TT_RIGHT_BRACKET) == NULL)
                return false;

            Token *name = parser_expect(TT_NA);
            if (name == NULL) 
                return false;

            if (!type->is_defined) {
                parser_log_error(&name->loc, 
                                 "only pointers can predifined types");
                return false;
            }

            if (type_array(name->lexeme, type, elements) == NULL) {
                parser_log_error(&name->loc, 
                                 "type with name %s already exists", 
                                 name->lexeme); 
                return false;    
            }

            return true;
        }

    case TT_STAR:
        {
            Token *name = parser_expect(TT_NA);
            if (name == NULL) 
                return false;

            if (type_pointer(name->lexeme, type) == NULL) {
                parser_log_error(&name->loc, 
                                 "type with name %s already exists", 
                                 name->lexeme); 
                return false;    
            }

            return true;
        }

    default:
        parser_log_error(&op->loc, "unexpected %s", 
                         op->lexeme); 
        return false;
    }
}

bool parser_tyds()
{
    bool first = true;

    Token *tydef = parser_get_token();
    while (tydef->type == TT_TYPEDEF) {
        if (!parser_tyd())
            return false;

        Token *semicolon = parser_get_token();
        if (semicolon->type != TT_SEMICOLON) {
            parser_unget_token();
            return true;
        }

        tydef = parser_get_token();
        first = false;
    }

    if (!first)
        parser_unget_token(); 

    parser_unget_token();
    return true;
}

bool parser_global_vad()
{
    Type *type = parser_ty(true);
    if (type == NULL) 
        return false;

    Token *name = parser_expect(TT_NA);
    if (name == NULL)
        return false;

    Symbol *sym = symbol_create(name->lexeme, 
                                type, 
                                SS_GLOBAL, 
                                &name->loc);
    if (!symtable_add(global_syms, sym)) {
        parser_log_error(&name->loc, 
                         "variable with name %s already exists", 
                         name->lexeme);
        free(sym);
        return false;
    }

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

static int parser_local_vads(SymTable *local)
{
    int count = 0;
    do {
        Token *type = parser_get_token();
        Token *name = parser_get_token();

        if (type->type == TT_RETURN || 
            type->type == TT_IF || 
            type->type == TT_WHILE || 
            name->type != TT_NA) {
            parser_unget_token();
            parser_unget_token();
            break;
        }

        if (!parser_is_type(type)) {
            parser_log_error(&type->loc,
                             "unknown type: %s",
                             type->lexeme);
            return -1;
        }


        count += 1;
        Symbol *sym = symbol_create(name->lexeme, 
                                    type_get(type->lexeme),
                                    SS_LOCAL,
                                    &name->loc); 
        if (!symtable_add(local, sym)) {
            parser_log_error(&name->loc, 
                             "variable with name %s already exists", 
                             name->lexeme);
            free(sym);
            return -1;
        }

    } while (parser_get_token()->type == TT_SEMICOLON);

    if (count > 0)
        parser_unget_token();

    return count;
}

Function *parser_fud()
{
    // Return type
    Type *return_type = parser_ty(true);
    if (return_type == NULL)
        return NULL;

    // Name
    Token *name_token = parser_expect(TT_NA);
    if (name_token == NULL)
        return NULL;

    char *fun_name = name_token->lexeme;

    // Arguments
    if (parser_expect(TT_LEFT_PAREN) == NULL) 
        return NULL;

    SymTable *local = symtable_create(global_syms);

    Type **arg_types = NULL;
    size_t arg_count = 0;

    Token *token = parser_get_token();
    switch (token->type) {
    case TT_RIGHT_PAREN:
        break;

    case TT_NA:
    case TT_BOOL:
    case TT_INT:
    case TT_CHAR:
    case TT_UINT:
        {
            parser_unget_token();

            size_t allocated = 8;
            arg_types = malloc(allocated * sizeof *arg_types);

            Token *t = NULL;
            do {
                Type *type = parser_ty(true);
                if (type == NULL)
                    goto clean_arg_types;

                Token *name = parser_get_token();

                Symbol *new = symbol_create(name->lexeme, 
                                            type,
                                            SS_LOCAL,
                                            &name->loc); 
                if (!symtable_add(local, new)) {
                    parser_log_error(&name->loc, 
                                     "parameter with name %s already exists",
                                     name->lexeme);
                    free(new);
                    goto clean_arg_types;
                }

                arg_types[arg_count++] = type;
                if (arg_count == allocated) {
                    allocated *= 2;
                    arg_types = realloc(arg_types, allocated * sizeof *arg_types);
                }

                t = parser_get_token();
            } while(t->type == TT_COMMA);

            if (t->type != TT_RIGHT_PAREN) {
                parser_log_error(&t->loc,
                                 "expected \")\" but got %s",
                                 t->lexeme);
                goto clean_arg_types;
            }

            arg_types = realloc(arg_types, arg_count * sizeof *arg_types);
        }
        break;

    default:
        parser_log_error(&token->loc, "unexpected %s", 
                         token->lexeme);
        goto clean_symtable;
    } 

    // Local variable declarations
    if (parser_expect(TT_LEFT_BRACE) == NULL) 
        goto clean_arg_types;

    int local_vads_result = parser_local_vads(local);
    if (local_vads_result == -1 ||
        (local_vads_result > 0 && parser_expect(TT_SEMICOLON) == NULL))
        goto clean_arg_types;


    // Statements
    Token *t = parser_get_token();
    parser_unget_token();

    Stmt **stmts = NULL;
    if (t->type != TT_RETURN) {
        stmts = parser_stmts();
        if (parser_expect(TT_SEMICOLON) == NULL) 
            goto clean_symtable;
    }

    // Return statement
    t = parser_expect(TT_RETURN);
    if (t == NULL) 
        goto clean_stmts;

    size_t start_column = t->loc.column_start;

    Expr *e = parser_cc_be_e();
    if (e == NULL)
        goto clean_stmts;

    Stmt *return_stmt = stmt_return(e, start_column);

    if (parser_expect(TT_RIGHT_BRACE) == NULL)
        goto clean_all;

    return function_create(fun_name, arg_types, arg_count, local, 
                           stmts, return_type, return_stmt);

clean_all:
    stmt_free(return_stmt);

clean_stmts:
    if (stmts != NULL)
        stmts_free(stmts); 

clean_arg_types:
    if (arg_types != NULL)
        free(arg_types);

clean_symtable:
    symtable_destroy(local);

    return NULL;
}
