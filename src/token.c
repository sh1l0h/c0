#include "../include/token.h"

char *token_strings[] = {
    "EOF", "true", "false", "null", "&", "!", "!=", "&&", "(", ")", "*", "+", 
    ",", "-", ".", "/", ";", ">", ">=", "=", "==", "<", "<=", "[", "]", "{", 
    "}", "||", "@", "char", "uint", "bool", "struct", "else", "int", "if", 
    "return", "typedef", "while", "new" 
};

Token *token_create(TokenType type, Location *loc_src)
{
    Token *result = calloc(1, sizeof *result);
    result->type = type;
    memcpy(&result->loc, loc_src, sizeof *loc_src);
    result->lexeme = token_strings[type];
    return result;
}

Token *token_create_with_lexeme(TokenType type, Location *loc_src, char *lexeme)
{
    Token *result = calloc(1, sizeof *result);
    result->type = type;
    memcpy(&result->loc, loc_src, sizeof *loc_src);
    result->lexeme = lexeme;
    return result;
}

void token_destroy(Token *token)
{
    free(token);
}

