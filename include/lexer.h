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

extern Lexer *lexer;

bool lexer_init(char *input_path);
void lexer_deinit();

Token *lexer_next();

#endif
