#ifndef C0_AST_H
#define C0_AST_H

#include "./token.h"
#include "./data_structures/array_list.h"

typedef enum ExprType {
    ET_BINARY,
    ET_UNARY,

    ET_ACCESS,
    ET_ARR_ACCESS,

    ET_C,
    ET_BC,
    ET_CC,
    ET_NA,
    ET_NULL
} ExprType;

typedef struct Expr Expr;

struct Expr {
    ExprType type;
    Location loc;

    bool has_value;
    TokenValue value;

    union {
        struct {
            TokenType op;
            Expr *left, *right;
        } binary;
        struct {
            TokenType op;
            Expr *e;
        } unary;
        struct {
            Expr *left;
            char *na;
        } access;
        struct {
            Expr *left, *index;
        } arr_access;
        bool bc;
        long c;
        char cc;
        char *na;
    } as;
};

typedef enum StmtType {
    ST_ASSIGN,
    ST_IF,
    ST_WHILE,
    ST_FUNCALL,
    ST_NEW,
    ST_RETURN
} StmtType;

typedef struct Stmt Stmt;

struct Stmt {
    StmtType type;
    Location loc;
    union {
        struct {
            Expr *left, *right;
        } assign;
        struct {
            Expr *cond;
            ArrayList *then_block, *else_block;
        } if_stmt;
        struct {
            Expr *cond;
            ArrayList *block;
        } while_stmt;
        struct {
            Expr *left;
            char *na;
            ArrayList *args;
        } funcall;
        struct {
            Expr *left;
            char *na;
        } new;
        Expr *return_stmt;
    } as;
};

Expr *expr_binary(TokenType op, Expr *left, Expr *right);
Expr *expr_unary(TokenType op, Expr *e, size_t column, bool is_prefix);
Expr *expr_access(Token *na, Expr *left);
Expr *expr_arr_access(Expr *left, Expr *index, size_t column_end);
Expr *expr_c(Token *c);
Expr *expr_bc(Token *bc);
Expr *expr_cc(Token *cc);
Expr *expr_null(Token *c);
Expr *expr_na(Token *na);

void expr_free(Expr *e);

Stmt *stmt_assign(Expr *left, Expr *right);
Stmt *stmt_if(Expr *cond, ArrayList *then_block, 
              ArrayList *else_block, size_t start_column);
Stmt *stmt_while(Expr *cond, ArrayList *block, size_t start_column);
Stmt *stmt_funcall(Expr *left, Token *na, ArrayList *args, size_t end_column);
Stmt *stmt_new(Expr *left, Token *na, size_t end_column);
Stmt *stmt_return(Expr *expr, size_t start_column);

void stmt_free(Stmt *stmt);

#endif
