#ifndef C0_AST_H
#define C0_AST_H

#include "./token.h"
#include "./data_structures/array_list.h"

typedef enum ExprType {
    ET_BINARY,
    ET_UNARY,

    ET_ACCESS,
    ET_ARR_ACCESS,

    ET_TOKEN,
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
            Token *na;
            Expr *left;
        } access;
        struct {
            Expr *left, *index;
        } arr_access;
        Token *token;
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
            Token *na;
            ArrayList *args;
        } funcall;
        Token *new;
        Expr *return_stmt;
    } as;
};

Expr *expr_binary(TokenType op, Expr *left, Expr *right);
Expr *expr_unary(TokenType op, Expr *e, size_t column, bool is_prefix);
Expr *expr_access(Token *na, Expr *left);
Expr *expr_arr_access(Expr *left, Expr *index, size_t column_end);
Expr *expr_token(Token *t);

void expr_free(Expr *e, bool free_tokens);

Stmt *stmt_assign(Expr *left, Expr *right);
Stmt *stmt_if(Expr *cond, ArrayList *then_block, ArrayList *else_block,
              size_t start_column, size_t end_column);
Stmt *stmt_while(Expr *cond, ArrayList *block,
                 size_t start_column, size_t end_column);
Stmt *stmt_funcall(Expr *left, Token *na, ArrayList *args, size_t end_column);
Stmt *stmt_new(Token *type, size_t start_column, size_t end_column);
Stmt *stmt_return(Expr *expr, size_t start_column);

void stmt_free(Stmt *stmt);

#endif
