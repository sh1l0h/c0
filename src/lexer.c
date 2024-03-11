#include "../include/lexer.h"

Lexer *lexer_alloc(char *input_path)
{
    Lexer *result = malloc(sizeof *result);

    result->input_stream = fopen(input_path, "r");
    if (result->input_stream == NULL) {
        log_fatal("%s: %s.", input_path, strerror(errno));

        free(result);
        return NULL;
    }

    result->input_path = input_path;
    result->line = result->column = 1;

    result->error = false;

    return result;
}

void lexer_free(Lexer *lexer)
{
    if (lexer->input_stream != NULL)
        fclose(lexer->input_stream);

    free(lexer);
}

static Token *lexer_num(Lexer *lexer, Location *loc)
{
    size_t allocated_chars = 8;
    size_t lexeme_len = 0;
    char *lexeme = malloc(allocated_chars * sizeof *lexeme);
    char curr;

    while (isdigit(curr = getc(lexer->input_stream))) {
        lexer->column++;
        lexeme[lexeme_len++] = curr;

        if (lexeme_len == allocated_chars)
            lexeme = realloc(lexeme, (allocated_chars *= 2) * sizeof *lexeme);
    }

    if (curr == 'u') {
        lexeme = realloc(lexeme, (lexeme_len + 2) * sizeof *lexeme);
        lexeme[lexeme_len] = 'u';
        lexeme[lexeme_len + 1] = '\0';
        lexer->column++;
    }
    else {
        lexeme = realloc(lexeme, (lexeme_len + 1) * sizeof *lexeme);
        lexeme[lexeme_len] = '\0';

        ungetc(curr, lexer->input_stream);
    }

    loc->column_end = lexer->column - 1;

    Token *result = token_alloc_with_lexeme(TT_C, loc, lexeme);

    result->value_as.integer = strtol(lexeme, NULL, 10);
    return result;
}

static Token *lexer_word(Lexer *lexer, Location *loc)
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

        if (lexeme_len == allocated_chars)
            lexeme = realloc(lexeme, (allocated_chars *= 2) * sizeof *lexeme);
    }

    ungetc(curr, lexer->input_stream);

    loc->column_end = lexer->column - 1;

    lexeme = realloc(lexeme, (lexeme_len + 1) * sizeof *lexeme);
    lexeme[lexeme_len] = '\0';

    Token *result;
    if (!strcmp(lexeme, "true")) {
        result = token_alloc_with_lexeme(TT_BC, loc, lexeme);
        result->value_as.boolean = true;
        return result;
    }

    if (!strcmp(lexeme, "false")) {
        result = token_alloc_with_lexeme(TT_BC, loc, lexeme);
        result->value_as.boolean = false;
        return result;
    }

    if (!strcmp(lexeme, "null")) {
        result = token_alloc_with_lexeme(TT_C, loc, lexeme);
        result->is_null = true;
        return result;
    }

    for (int i = 0; i < TT_KEYWORD_COUNT - TT_CHAR; i++) {
        if (!strcmp(lexeme, token_type_strings[TT_CHAR + i])) {
            free(lexeme);
            return token_alloc(TT_CHAR + i, loc);
        }
    }

    return token_alloc_with_lexeme(TT_Na, loc, lexeme);
}

static bool lexer_match(Lexer *lexer, const int c)
{
    int curr = getc(lexer->input_stream);

    if (curr != c) {
        ungetc(curr, lexer->input_stream);
        return false;
    }

    lexer->column++;

    return true;
}

Token *lexer_next(Lexer *lexer)
{
    bool quit;
    int curr;
    Location loc;
    Token *result;
    size_t last_column = 0;

    loc.file_path = lexer->input_path;

    do {
        quit = true;

        curr = getc(lexer->input_stream);

        loc.column_start = loc.column_end = lexer->column;
        loc.line = lexer->line;

        if (isalpha(curr)) {
            ungetc(curr, lexer->input_stream);
            result = lexer_word(lexer, &loc);
            break;
        }

        if (isdigit(curr)) {
            ungetc(curr, lexer->input_stream);
            result = lexer_num(lexer, &loc);
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
            result = token_alloc(TT_EOF, &loc);
            break;

        case '&':
            if (lexer_match(lexer, '&')) {
                loc.column_end++;
                result = token_alloc(TT_LOGICAL_AND, &loc);
            }
            else
                result = token_alloc(TT_AND, &loc);
            break;

        case '!':
            if (lexer_match(lexer, '=')) {
                loc.column_end++;
                result = token_alloc(TT_NOT_EQUALS, &loc);
            }
            result = token_alloc(TT_NOT, &loc);
            break;

        case '(':
            result = token_alloc(TT_LEFT_PAREN, &loc);
            break;

        case ')':
            result = token_alloc(TT_RIGHT_PAREN, &loc);
            break;

        case '*':
            result = token_alloc(TT_STAR, &loc);
            break;

        case '+':
            result = token_alloc(TT_PLUS, &loc);
            break;

        case ',':
            result = token_alloc(TT_COMMA, &loc);
            break;

        case '-':
            result = token_alloc(TT_MINUS, &loc);
            break;

        case '.':
            result = token_alloc(TT_DOT, &loc);
            break;

        case '/':
            result = token_alloc(TT_SLASH, &loc);
            break;

        case ';':
            result = token_alloc(TT_SEMICOLON, &loc);
            break;

        case '>':
            if (lexer_match(lexer, '=')) {
                loc.column_end++;
                result = token_alloc(TT_GREATER_EQUALS, &loc);
            }
            else
                result = token_alloc(TT_GREATER, &loc);
            break;

        case '=':
            if (lexer_match(lexer, '=')) {
                loc.column_end++;
                result = token_alloc(TT_LOGICAL_EQUALS, &loc);
            }
            else 
                result = token_alloc(TT_EQUALS, &loc);
            break;

        case '<':
            if (lexer_match(lexer, '=')) {
                loc.column_end++;
                result = token_alloc(TT_LESS_EQUALS, &loc);
            }
            else
                result = token_alloc(TT_LESS, &loc);
            break;

        case '[':
            result = token_alloc(TT_LEFT_BRACKET, &loc);
            break;

        case ']':
            result = token_alloc(TT_RIGHT_BRACKET, &loc);
            break;

        case '{':
            result = token_alloc(TT_LEFT_BRACE, &loc);
            break;

        case '}':
            result = token_alloc(TT_RIGHT_BRACE, &loc);
            break;

        case '|':
            if (!lexer_match(lexer, '|')) {
                log_error_with_loc(&loc, "did you mean \"||\"?");
                log_info("treated \"|\" as \"||\" and continuing...");
                lexer->error = true;
            }
            loc.column_end++;
            result = token_alloc(TT_LOGICAL_OR, &loc);
            break;

        case '@':
            result = token_alloc(TT_AT, &loc);
            break;

        default:
            log_error_with_loc(&loc, "unexpected character \"%c\".", curr);
            log_info("skipped the character and continuing...");
            quit = false;
            lexer->error = true;
        }

        lexer->column++;

    } while (!quit);

    return result;
}
