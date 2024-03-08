#ifndef C0_LEXER_H
#define C0_LEXER_H

#include "./utils.h"
#include "./token.h"

typedef struct Lexer {
    FILE *input_stream;
    char *input_path;
    size_t line, column;
    bool error;
} Lexer;

Lexer *lexer_alloc(char *input_path);
void lexer_free(Lexer *lexer);

Token *lexer_next(Lexer *lexer);

#endif
