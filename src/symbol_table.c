#include "../include/symbol_table.h"

SymTable *global_syms = NULL;

Symbol *symbol_create(char *name, Type *type, SymScope scope, Location *loc_src)
{
    Symbol *result = calloc(1, sizeof *result);
    result->name = name;
    result->type = type;
    result->scope = scope;

    if (loc_src != NULL)
        memcpy(&result->loc, loc_src, sizeof *loc_src);

    return result;
}

void symtable_init()
{
    global_syms = symtable_create(NULL);
}

void symtable_deinit()
{
    symtable_destroy(global_syms);
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
        while (table->symbols[i] != NULL) {
            Symbol *tmp = table->symbols[i];
            table->symbols[i] = tmp->next;
            free(tmp);
        }
    }

    free(table);
}

void symtable_add(SymTable *table, Symbol *sym)
{
    size_t index = str_hash(sym->name) & (SYMTABLE_SIZE - 1);
    sym->next = table->symbols[index];
    table->symbols[index] = sym;
}

Symbol *symtable_get(SymTable *table, char *name)
{
    size_t index = str_hash(name) & (SYMTABLE_SIZE - 1);
    
    for (Symbol *curr = table->symbols[index]; 
         curr != NULL; 
         curr = curr->next) { 
        if (!strcmp(curr->name, name))
            return curr;
    }

    return NULL;
}
