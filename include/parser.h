#ifndef C0_PARSER_H
#define C0_PARSER_H

#include "./data_structures/cyclic_queue.h"
#include "./lexer.h"
#include "./ast.h"
#include "./type.h"

#define PARSER_LOOK_AHEAD 3

typedef struct Parser {
    Lexer *lexer;
    bool error;

    bool is_tracking;
    size_t curr_token;
    CyclicQueue tokens;
} Parser;

typedef struct ParserState {
    size_t start_index;
    bool is_first;
} ParserState;

extern Parser *parser;

void parser_init();
void parser_deinit();

Expr *parser_id();
Expr *parser_f();
Expr *parser_t();
Expr *parser_e();

Expr *parser_bf();
Expr *parser_bt();
Expr *parser_be();

Stmt *parser_stmt();

Type *parser_ty(bool should_exist);
bool parser_tyd();
bool parser_tyds();

bool parser_global_vad();

Function *parser_fud();

#endif
