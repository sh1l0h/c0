#include <string.h>
#include "../include/ast.h"
#include "../include/symbol_table.h"

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
    result->as.access.na = na->lexeme;

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
    result->as.na = na->lexeme;
    result->loc = na->loc;

    return result;
}

void exprs_free(Expr **exprs)
{
    for (size_t i = 0; exprs[i] != NULL; i++) 
        expr_free(exprs[i]);

    free(exprs);
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
        break;

    case ET_ARR_ACCESS:
        expr_free(e->as.arr_access.index);
        expr_free(e->as.arr_access.left);
        break;
    
    default:
        break;
    }

    free(e);
}

void expr_free_wrapper(void *e)
{
    expr_free(e);
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

Stmt *stmt_if(Expr *cond, Stmt **then_block, 
              Stmt **else_block, size_t start_column)
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

Stmt *stmt_while(Expr *cond, Stmt **block, size_t start_column)
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

Stmt *stmt_funcall(Expr *left, Token *na, Expr **args, size_t end_column)
{
    Stmt *result = malloc(sizeof *result);
    result->type = ST_FUNCALL;
    result->as.funcall.left = left;
    result->as.funcall.na = na->lexeme;
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
    result->as.new_stmt.na = na->lexeme;

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

void stmts_free(Stmt **stmts)
{
    for (size_t i = 0; stmts[i] != NULL; i++)
        stmt_free(stmts[i]);

    free(stmts);
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

            stmts_free(stmt->as.if_stmt.then_block);

            Stmt **else_block = stmt->as.if_stmt.else_block;
            if (else_block == NULL)
                break;
            
            stmts_free(else_block);
        }
        break;

    case ST_WHILE:
        expr_free(stmt->as.while_stmt.cond);
        stmts_free(stmt->as.while_stmt.block);
        break;

    case ST_FUNCALL:
        expr_free(stmt->as.funcall.left); 
        exprs_free(stmt->as.funcall.args);
        break;

    case ST_NEW:
        expr_free(stmt->as.new_stmt.left);
        break;

    case ST_RETURN:
        expr_free(stmt->as.return_stmt);
        break;
    }
    free(stmt);
}

void stmt_free_wrapper(void *stmt)
{
    stmt_free(stmt); 
}

Function *function_create(char *name, Type **arg_types, 
                          size_t arg_count, SymTable *table, 
                          Stmt **stmts, Type *return_type, 
                          Stmt *return_stmt)
{
    Function *result = malloc(sizeof *result);
    result->name = name;
    result->arg_types = arg_types;
    result->arg_count = arg_count;
    result->table = table;
    result->stmts = stmts;
    result->return_type = return_type;
    result->return_stmt = return_stmt;

    //Symbol *funsym = function_symbol_create(name, result, NULL);

    return result;
}

void function_free(Function *fun)
{
    free(fun->arg_types);
    symtable_destroy(fun->table);
    if (fun->stmts != NULL)
        stmts_free(fun->stmts);
    stmt_free(fun->return_stmt);
    free(fun);
}
