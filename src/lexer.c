#include "../include/lexer.h"

Lexer *lexer;

bool lexer_init(char *input_path)
{
    lexer = malloc(sizeof *lexer);

    lexer->input_stream = fopen(input_path, "r");
    if (lexer->input_stream == NULL) {
        log_fatal("%s: %s.", input_path, strerror(errno));

        free(lexer);
        return false;
    }

    lexer->input_path = input_path;
    lexer->line = 1;
    lexer->column = 1;
    lexer->error = false;

    return true;
}

void lexer_deinit()
{
    if (lexer->input_stream != NULL)
        fclose(lexer->input_stream);

    free(lexer);
}

static Token *lexer_num(Location *loc)
{
    size_t allocated_chars = 64;
    size_t lexeme_len = 0;
    char *lexeme = malloc(allocated_chars * sizeof *lexeme);
    char curr;

    while (isdigit(curr = getc(lexer->input_stream))) {
        lexer->column++;
        lexeme[lexeme_len++] = curr;

        if (lexeme_len == allocated_chars) {
            allocated_chars *= 2;
            char *tmp = realloc(lexeme, allocated_chars * sizeof *lexeme);
            if (tmp == NULL)
                return NULL;
            lexeme = tmp;
        }
    }

    if (curr != 'u') 
        ungetc(curr, lexer->input_stream);
    else {
        lexeme = realloc(lexeme, (lexeme_len + 1) * sizeof *lexeme);
        lexeme[lexeme_len++] = 'u';
        lexer->column++;
    }

    loc->column_end = lexer->column - 1;

    char *str = str_get(lexeme, lexeme_len);
    free(lexeme);

    Token *result = token_create_with_lexeme(TT_C, loc, str);
    result->value_as.integer = strtol(str, NULL, 10);

    return result;
}

static Token *lexer_word(Location *loc)
{
    size_t allocated_chars = 8;
    size_t lexeme_len = 0;
    char *lexeme = malloc(allocated_chars * sizeof *lexeme);
    char curr;

    while (((curr = getc(lexer->input_stream)) == '_' ||
            isalpha(curr) ||
            isdigit(curr))) {
        lexer->column++;
        lexeme[lexeme_len++] = curr;

        if (lexeme_len == allocated_chars) {
            char *tmp = realloc(lexeme, (allocated_chars *= 2) * sizeof *lexeme);
            if (tmp == NULL)
                return NULL;
            lexeme = tmp;
        }
    }

    ungetc(curr, lexer->input_stream);

    loc->column_end = lexer->column - 1;
    lexeme = realloc(lexeme, (lexeme_len + 1) * sizeof *lexeme);
    lexeme[lexeme_len] = '\0';

    Token *result;
    if (!strcmp(lexeme, "true")) {
        result = token_create_with_lexeme(TT_BC, loc, token_strings[TT_TRUE]);
        result->value_as.boolean = true;
        free(lexeme);
        return result;
    }

    if (!strcmp(lexeme, "false")) {
        result = token_create_with_lexeme(TT_BC, loc, token_strings[TT_FALSE]);
        result->value_as.boolean = false;
        free(lexeme);
        return result;
    }

    if (!strcmp(lexeme, "null")) {
        result = token_create_with_lexeme(TT_C, loc, token_strings[TT_NULL]);
        result->is_null = true;
        free(lexeme);
        return result;
    }

    for (int i = TT_CHAR; i < TT_KEYWORD_COUNT; i++) {
        if (!strcmp(lexeme, token_strings[i])) {
            free(lexeme);
            return token_create_with_lexeme(i, loc, token_strings[i]);
        }
    }

    char *s = str_get_null_term(lexeme);
    free(lexeme);

    return token_create_with_lexeme(TT_NA, loc, s);
}

static bool lexer_match(const int c)
{
    int curr = getc(lexer->input_stream);

    if (curr != c) {
        ungetc(curr, lexer->input_stream);
        return false;
    }

    lexer->column++;

    return true;
}

Token *lexer_next()
{
    bool quit;
    Location loc;
    Token *result;
    size_t last_column = 0;

    loc.file_path = lexer->input_path;

    do {
        quit = true;

        int curr = getc(lexer->input_stream);

        loc.column_start = loc.column_end = lexer->column;
        loc.line = lexer->line;

        if (isalpha(curr)) {
            ungetc(curr, lexer->input_stream);
            result = lexer_word(&loc);
            break;
        }

        if (isdigit(curr)) {
            ungetc(curr, lexer->input_stream);
            result = lexer_num(&loc);
            break;
        }

        switch (curr) {
        case ' ':
        case '\t':
            quit = false;
            break;

        case '\r':
        case '\n':
            lexer->line++;
            last_column = lexer->column + 1;
            lexer->column = 0;
            quit = false;
            break;

        case EOF:
            loc.line--;
            loc.column_start = loc.column_end = last_column;
            result = token_create(TT_EOF, &loc);
            break;

        case '&':
            if (lexer_match('&')) {
                loc.column_end++;
                result = token_create(TT_LOGICAL_AND, &loc);
            }
            else
                result = token_create(TT_AND, &loc);
            break;

        case '!':
            if (lexer_match('=')) {
                loc.column_end++;
                result = token_create(TT_NOT_EQUALS, &loc);
            }
            else
                result = token_create(TT_NOT, &loc);
            break;

        case '(':
            result = token_create(TT_LEFT_PAREN, &loc);
            break;

        case ')':
            result = token_create(TT_RIGHT_PAREN, &loc);
            break;

        case '*':
            result = token_create(TT_STAR, &loc);
            break;

        case '+':
            result = token_create(TT_PLUS, &loc);
            break;

        case ',':
            result = token_create(TT_COMMA, &loc);
            break;

        case '-':
            result = token_create(TT_MINUS, &loc);
            break;

        case '.':
            result = token_create(TT_DOT, &loc);
            break;

        case '/':
            result = token_create(TT_SLASH, &loc);
            break;

        case ';':
            result = token_create(TT_SEMICOLON, &loc);
            break;

        case '>':
            if (lexer_match('=')) {
                loc.column_end++;
                result = token_create(TT_GREATER_EQUALS, &loc);
            }
            else
                result = token_create(TT_GREATER, &loc);
            break;

        case '=':
            if (lexer_match('=')) {
                loc.column_end++;
                result = token_create(TT_LOGICAL_EQUALS, &loc);
            }
            else 
                result = token_create(TT_EQUALS, &loc);
            break;

        case '<':
            if (lexer_match('=')) {
                loc.column_end++;
                result = token_create(TT_LESS_EQUALS, &loc);
            }
            else
                result = token_create(TT_LESS, &loc);
            break;

        case '[':
            result = token_create(TT_LEFT_BRACKET, &loc);
            break;

        case ']':
            result = token_create(TT_RIGHT_BRACKET, &loc);
            break;

        case '{':
            result = token_create(TT_LEFT_BRACE, &loc);
            break;

        case '}':
            result = token_create(TT_RIGHT_BRACE, &loc);
            break;

        case '|':
            if (!lexer_match('|')) {
                log_error_with_loc(&loc, "did you mean \"||\"?");
                lexer->error = true;
            }
            loc.column_end++;
            result = token_create(TT_LOGICAL_OR, &loc);
            break;

        case '@':
            result = token_create(TT_AT, &loc);
            break;

        default:
            log_error_with_loc(&loc, "unexpected character \"%c\".", curr);
            quit = false;
            lexer->error = true;
        }

        lexer->column++;

    } while (!quit);

    return result;
}
