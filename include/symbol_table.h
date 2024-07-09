#ifndef C0_SYMBOL_TABLE_H
#define C0_SYMBOL_TABLE_H

#include "./utils.h"
#include "./type.h"

#define SYMTABLE_SIZE 128 // Always power of 2

typedef enum SymScope {
    SS_LABEL,
    SS_GLOBAL,
    SS_LOCAL
} SymScope;

typedef struct Symbol Symbol;
typedef struct SymTable SymTable;

typedef struct Function Function;

struct Symbol {
    char *name;
    Type *type;
    Function *function;
    SymScope scope;
    Location loc;

    Symbol *next;
};

struct SymTable {
    Symbol *symbols[SYMTABLE_SIZE];
    SymTable *prev;
};

extern SymTable *global_syms;
extern SymTable *function_syms;

Symbol *symbol_create(char *name, 
                      Type *type, 
                      SymScope scope, 
                      Location *loc_src);
Symbol *function_symbol_create(char *name, 
                               Function *function, 
                               Location *loc_src);

void symtable_init();
void symtable_deinit();

SymTable *symtable_create(SymTable *prev);
void symtable_destroy(SymTable *table);

bool symtable_add(SymTable *table, Symbol *sym);
Symbol *symtable_get(SymTable *table, char *name);

#endif
