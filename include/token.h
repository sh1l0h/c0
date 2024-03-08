#ifndef BANIC_TOKEN_H
#define BANIC_TOKEN_H

#include "./utils.h"

// DO NOT CHANGE THE ORDER!
typedef enum TokenType {
    TT_EOF = 0,

    TT_Na,

    TT_C,
    TT_CC,
    TT_BC,

    TT_AND,
    TT_NOT,
    TT_NOT_EQUALS,
    TT_LOGICAL_AND,
    TT_LEFT_PAREN,
    TT_RIGHT_PAREN,
    TT_STAR,
    TT_PLUS,
    TT_COMMA,
    TT_MINUS,
    TT_DOT,
    TT_SLASH,
    TT_SEMICOLON,
    TT_GREATER,
    TT_GREATER_EQUALS,
    TT_EQUALS,
    TT_LOGICAL_EQUALS,
    TT_LESS,
    TT_LESS_EQUALS,
    TT_LEFT_BRACKET,
    TT_RIGHT_BRACKET,
    TT_LEFT_BRACE,
    TT_RIGHT_BRACE,
    TT_LOGICAL_OR,
    TT_AT,

    TT_CHAR,
    TT_UINT,
    TT_BOOL,
    TT_STRUCT,
    TT_ELSE,
    TT_INT,
    TT_IF,
    TT_RETURN,
    TT_TYPEDEF,
    TT_WHILE,
    TT_NEW,
    TT_KEYWORD_COUNT
    
} TokenType;

typedef union TokenValue {
    long integer;
    char character;
    bool boolean;
} TokenValue;

typedef struct Token {
    TokenType type;
    Location loc;
    char *lexeme;

    bool is_null;
    TokenValue value_as;

    bool is_owned;
} Token;

extern const char *token_type_strings[];

Token *token_alloc(TokenType type, Location *loc_src);
Token *token_alloc_with_lexeme(TokenType type, Location *loc_src, char *lexeme);

void token_free(Token *token);

const char *token_to_string(Token *t);

#endif
