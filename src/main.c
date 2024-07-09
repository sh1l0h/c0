#include "../include/parser.h"
#include "../include/type.h"
#include "../include/symbol_table.h"

int main(int argc, char **argv)
{
    (void) argc;

    log_init(true);

    type_init();
    symtable_init();

    lexer_init(argv[1]);
    parser_init();

    Function *f = parser_fud(parser);

    if (f != NULL)
        function_free(f);

    parser_deinit();
    symtable_deinit();
    type_deinit();

    return 0;
}
