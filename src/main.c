#include "../include/parser.h"
#include "../include/type.h"
#include "../include/symbol_table.h"

int main(int argc, char **argv)
{
    (void) argc;

    log_init(true);

    type_init();
    symtable_init();

    Lexer *lexer = lexer_alloc(argv[1]);
    Parser *parser = parser_alloc(lexer);

    Function *f = parser_fud(parser);

    if (f != NULL)
        function_free(f);

    parser_free(parser);
    symtable_deinit();
    type_deinit();

    return 0;
}
