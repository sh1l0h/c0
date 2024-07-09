#include "../include/symbol_table.h"

SymTable *global_syms = NULL;
SymTable *function_syms = NULL;

static Symbol *symtable_get_locally(SymTable *table, char *name)
{
    size_t index = str_hash(name) & (SYMTABLE_SIZE - 1);
    
    for (Symbol *curr = table->symbols[index]; 
         curr != NULL; 
         curr = curr->next) { 
        if (curr->name == name)
            return curr;
    }

    return NULL;
}

Symbol *symbol_create(char *name, 
                      Type *type, 
                      SymScope scope, 
                      Location *loc_src)
{
    Symbol *result = calloc(1, sizeof *result);
    result->name = name;
    result->type = type;
    result->scope = scope;

    if (loc_src != NULL)
        memcpy(&result->loc, loc_src, sizeof *loc_src);

    return result;
}

Symbol *function_symbol_create(char *name, 
                               Function *function, 
                               Location *loc_src)
{
    Symbol *result = calloc(1, sizeof *result);
    result->name = name;
    result->scope = SS_GLOBAL;
    result->function = function;
    memcpy(&result->loc, loc_src, sizeof *loc_src);
    return result;
}

void symtable_init()
{
    global_syms = symtable_create(NULL);
    function_syms = symtable_create(NULL);
}

void symtable_deinit()
{
    symtable_destroy(global_syms);
    symtable_destroy(function_syms);
}

SymTable *symtable_create(SymTable *prev)
{
    SymTable *result = calloc(1, sizeof *result);
    result->prev = prev;

    return result;
}

void symtable_destroy(SymTable *table)
{
    for (size_t i = 0; i < SYMTABLE_SIZE; i++) {
        Symbol *curr = table->symbols[i];
        while (curr != NULL) {
            Symbol *tmp = curr;
            curr = curr->next;
            free(tmp);
        }
    }

    free(table);
}

bool symtable_add(SymTable *table, Symbol *sym)
{
    if (symtable_get_locally(table, sym->name) != NULL)
        return false;
    
    size_t index = str_hash(sym->name) & (SYMTABLE_SIZE - 1);
    sym->next = table->symbols[index];
    table->symbols[index] = sym;
    return true;
}

Symbol *symtable_get(SymTable *table, char *name)
{
    SymTable *curr = table;
    while (curr != NULL) {
        Symbol *sym = symtable_get_locally(curr, name);
        if (sym != NULL)
            return sym;
        curr = curr->prev;
    }

    return NULL;
}
