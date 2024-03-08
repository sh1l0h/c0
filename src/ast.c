#include "../include/ast.h"

static Expr *expr_alloc(ExprType type)
{
    Expr *result = malloc(sizeof *result);
    result->type = type;
    return result;
}

Expr *expr_binary(TokenType op, Expr *left, Expr *right)
{
    Expr *result = expr_alloc(ET_BINARY);
    result->as.binary.op = op;
    result->as.binary.left = left;
    result->as.binary.right = right;

    result->loc.file_path = left->loc.file_path;
    result->loc.line = left->loc.line;
    result->loc.column_start = left->loc.column_start;
    result->loc.column_end = right->loc.column_end;

    return result;
}

Expr *expr_unary(TokenType op, Expr *e, size_t column, bool is_prefix)
{
    Expr *result = expr_alloc(ET_UNARY);
    result->as.unary.op = op;
    result->as.unary.e = e;
    
    result->loc.file_path = e->loc.file_path;
    result->loc.line = e->loc.line;
    result->loc.column_start = is_prefix ? column : e->loc.column_start;
    result->loc.column_end = is_prefix ? e->loc.column_end : column;
    
    return result;
}

Expr *expr_access(Token *na, Expr *left)
{
    Expr *result = expr_alloc(ET_ACCESS);
    na->is_owned = true;
    result->as.access.na = na;
    result->as.access.left = left;

    result->loc.file_path = left->loc.file_path;
    result->loc.line = left->loc.line;
    result->loc.column_start = left->loc.column_start;
    result->loc.column_end = na->loc.column_end;

    return result;
}

Expr *expr_arr_access(Expr *left, Expr *index, size_t column_end)
{
    Expr *result = expr_alloc(ET_ARR_ACCESS);
    result->as.arr_access.left = left;
    result->as.arr_access.index = index;

    result->loc.file_path = left->loc.file_path;
    result->loc.line = left->loc.line;
    result->loc.column_start = left->loc.column_start;
    result->loc.column_end = column_end;

    return result;
}

Expr *expr_token(Token *t)
{
    Expr *result = expr_alloc(ET_TOKEN);
    result->as.token = t;

    result->loc = t->loc;

    t->is_owned = true;
        
    return result;
}

void expr_free(Expr *e, bool free_tokens)
{
    switch (e->type) {

    case ET_BINARY:
        expr_free(e->as.binary.left, free_tokens);
        expr_free(e->as.binary.right, free_tokens);
        break;

    case ET_UNARY:
        expr_free(e->as.unary.e, free_tokens);
        break;

    case ET_ACCESS:
        expr_free(e->as.access.left, free_tokens);
        if (free_tokens)
            token_free(e->as.access.na);
        else
            e->as.access.na->is_owned = false;
        break;

    case ET_ARR_ACCESS:
        expr_free(e->as.arr_access.index, free_tokens);
        expr_free(e->as.arr_access.left, free_tokens);
        break;

    case ET_TOKEN:
        if (free_tokens)
            token_free(e->as.token);
        else
            e->as.token->is_owned = false;
        break;
    }

    free(e);
}

Stmt *stmt_assign(Expr *left, Expr *right)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_ASSIGN;
    result->as.assign.left = left;
    result->as.assign.right = right;

    result->loc.line = left->loc.line;
    result->loc.file_path = left->loc.file_path;
    result->loc.column_start = left->loc.column_start;
    result->loc.column_end = right->loc.column_end;
    
    return result;
}

Stmt *stmt_if(Expr *cond, ArrayList *then_block, ArrayList *else_block,
              size_t start_column, size_t end_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_IF;
    result->as.if_stmt.cond = cond;
    result->as.if_stmt.then_block = then_block;
    result->as.if_stmt.else_block = else_block;

    result->loc.line = cond->loc.line;
    result->loc.file_path = cond->loc.file_path;
    result->loc.column_start = start_column;
    result->loc.column_end = end_column;

    return result;
}

Stmt *stmt_while(Expr *cond, ArrayList *block,
                 size_t start_column, size_t end_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_WHILE;
    result->as.while_stmt.cond = cond;
    result->as.while_stmt.block = block;

    result->loc.line = cond->loc.line;
    result->loc.file_path = cond->loc.file_path;
    result->loc.column_start = start_column;
    result->loc.column_end = end_column;

    return result;
}

Stmt *stmt_funcall(Expr *left, Token *na, ArrayList *args, size_t end_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_FUNCALL;
    result->as.funcall.left = left;
    result->as.funcall.na = na;
    na->is_owned = true;
    result->as.funcall.args = args;

    result->loc.line = left->loc.line;
    result->loc.file_path = left->loc.file_path;
    result->loc.column_start = left->loc.column_start;
    result->loc.column_end = end_column;

    return result;
}

Stmt *stmt_new(Expr *left, Token *na, size_t end_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_NEW;
    result->as.new.left = left;
    result->as.new.na = na;
    na->is_owned = true;

    result->loc.line = left->loc.line;
    result->loc.file_path = left->loc.file_path;
    result->loc.column_start = left->loc.column_start;
    result->loc.column_end = end_column;

    return result;
}

Stmt *stmt_return(Expr *expr, size_t start_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_RETURN;
    result->as.return_stmt = expr;

    result->loc.line = expr->loc.line;
    result->loc.file_path = expr->loc.file_path;
    result->loc.column_start = start_column;
    result->loc.column_end = expr->loc.column_end;

    return result;
}


void stmt_free(Stmt *stmt)
{
    // TODO: implement this function
    log_error("stmt_free unimplemented");
}
