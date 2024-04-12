#include <string.h>
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
    result->as.access.left = left;
    result->as.access.na = str_dup(na->lexeme);

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

Expr *expr_c(Token *c)
{
    Expr *result = expr_alloc(ET_C);
    result->as.c = c->value_as.integer;
    result->loc = c->loc;

    return result;
}

Expr *expr_bc(Token *bc)
{
    Expr *result = expr_alloc(ET_BC);
    result->as.bc = bc->value_as.boolean;
    result->loc = bc->loc;

    return result;
}

Expr *expr_cc(Token *cc)
{
    Expr *result = expr_alloc(ET_CC);
    result->as.cc = cc->value_as.character;
    result->loc = cc->loc;

    return result;
}

Expr *expr_null(Token *c)
{
    Expr *result = expr_alloc(ET_NULL);
    result->loc = c->loc;
    return result;
}

Expr *expr_na(Token *na)
{
    Expr *result = expr_alloc(ET_NA);
    result->as.na = str_dup(na->lexeme);
    result->loc = na->loc;

    return result;
}

void expr_free(Expr *e)
{
    switch (e->type) {

    case ET_BINARY:
        expr_free(e->as.binary.left);
        expr_free(e->as.binary.right);
        break;

    case ET_UNARY:
        expr_free(e->as.unary.e);
        break;

    case ET_ACCESS:
        expr_free(e->as.access.left);
        free(e->as.access.na);
        break;

    case ET_ARR_ACCESS:
        expr_free(e->as.arr_access.index);
        expr_free(e->as.arr_access.left);
        break;
    
    case ET_NA:
        free(e->as.na);
        break;

    default:
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
              size_t start_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_IF;
    result->as.if_stmt.cond = cond;
    result->as.if_stmt.then_block = then_block;
    result->as.if_stmt.else_block = else_block;

    result->loc.line = cond->loc.line;
    result->loc.file_path = cond->loc.file_path;
    result->loc.column_start = start_column;
    result->loc.column_end = cond->loc.column_end;

    return result;
}

Stmt *stmt_while(Expr *cond, ArrayList *block,
                 size_t start_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_WHILE;
    result->as.while_stmt.cond = cond;
    result->as.while_stmt.block = block;

    result->loc.line = cond->loc.line;
    result->loc.file_path = cond->loc.file_path;
    result->loc.column_start = start_column;
    result->loc.column_end = cond->loc.column_end;

    return result;
}

Stmt *stmt_funcall(Expr *left, Token *na, ArrayList *args, size_t end_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_FUNCALL;
    result->as.funcall.left = left;
    result->as.funcall.na = str_dup(na->lexeme);
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
    result->as.new_stmt.left = left;
    result->as.new_stmt.na = str_dup(na->lexeme);

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
    switch (stmt->type) {

    case ST_ASSIGN:
        expr_free(stmt->as.assign.left);
        expr_free(stmt->as.assign.right);
        break;

    case ST_IF:
        {
            expr_free(stmt->as.if_stmt.cond);

            ArrayList *then_block = stmt->as.if_stmt.then_block;
            for (size_t i = 0; i < then_block->size; i++) 
                stmt_free(*(Stmt **) array_list_offset(then_block, i));
            array_list_destroy(then_block);
            free(then_block);

            ArrayList *else_block = stmt->as.if_stmt.else_block;
            if (else_block == NULL)
                break;

            for (size_t i = 0; i < else_block->size; i++) 
                stmt_free(*(Stmt **) array_list_offset(else_block, i));
            array_list_destroy(else_block);
            free(else_block);
        }
        break;

    case ST_WHILE:
        {
            expr_free(stmt->as.while_stmt.cond);

            ArrayList *block = stmt->as.while_stmt.block;
            for (size_t i = 0; i < block->size; i++) 
                stmt_free(*(Stmt **) array_list_offset(block, i));
            array_list_destroy(block);
            free(block);
        }
        break;

    case ST_FUNCALL:
        {
            expr_free(stmt->as.funcall.left); 
            free(stmt->as.funcall.na);

            ArrayList *args = stmt->as.funcall.args;
            for (size_t i = 0; i < args->size; i++) 
                expr_free(*(Expr **) array_list_offset(args, i));
            array_list_destroy(args);
        }
        break;

    case ST_NEW:
        expr_free(stmt->as.new_stmt.left);
        free(stmt->as.new_stmt.na);
        break;

    case ST_RETURN:
        expr_free(stmt->as.return_stmt);
        break;
    }
    free(stmt);
}
