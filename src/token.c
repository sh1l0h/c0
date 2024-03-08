#include "../include/token.h"

Token *token_alloc(TokenType type, Location *loc_src)
{
    Token *result = calloc(1, sizeof *result);
    result->type = type;
    memcpy(&result->loc, loc_src, sizeof *loc_src);
    result->is_owned = false;
    return result;
}

Token *token_alloc_with_lexeme(TokenType type, Location *loc_src, char *lexeme)
{
    Token *result = token_alloc(type, loc_src);
    result->lexeme = lexeme;

    return result;
}

void token_free(Token *token)
{
    if (token->lexeme != NULL)
        free(token->lexeme);

    free(token);
}

const char *token_type_strings[] = {
    "EOF", "name", "constant", "constant char", "boolean constant", "&", "!",
    "!=", "&&", "(", ")", "*", "+", ",", "-", ".", "/", ";", ">", ">=", "=",
    "==", "<", "<=", "[", "]", "{", "}", "||", "@", "char", "uint", "bool",
    "struct", "else", "int", "if", "return", "typedef", "while", "new"
};

const char *token_to_string(Token *t)
{
    if (t->lexeme != NULL)
        return t->lexeme;

    return token_type_strings[t->type];
}
