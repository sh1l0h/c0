#ifndef C0_PARSER_H
#define C0_PARSER_H

#include "./data_structures/cyclic_queue.h"
#include "./lexer.h"
#include "./ast.h"

#define PARSER_LOOK_AHEAD 1

typedef struct Parser {
    Lexer *lexer;
    bool error;

    bool is_tracking;
    size_t curr_token;
    CyclicQueue tokens;
} Parser;

Parser *parser_alloc(Lexer *lexer);
void parser_free(Parser *parser);

Expr *parser_id(Parser *parser);
Expr *parser_f(Parser *parser);
Expr *parser_t(Parser *parser);
Expr *parser_e(Parser *parser);

Expr *parser_bf(Parser *parser);
Expr *parser_bt(Parser *parser);
Expr *parser_be(Parser *parser);

Stmt *parser_assignment(Parser *parser);
#endif
