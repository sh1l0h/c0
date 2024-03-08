#include "../include/parser.h"

int main(int argc, char **argv)
{
    Lexer *lexer = lexer_alloc(argv[1]);
    Parser *parser = parser_alloc(lexer);

    Stmt *s = parser_assignment(parser);
    parser_free(parser);

    if (s != NULL) {
        log_info_with_loc(&s->loc, "");
    }

    return 0;
}
