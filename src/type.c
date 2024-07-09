#include "../include/type.h"
#include "../include/token.h"

#define TYPE_TABLE_SIZE 128 // Always power of 2

static Type *type_table[TYPE_TABLE_SIZE] = {NULL};

Type *type_int;
Type *type_bool;
Type *type_char;
Type *type_uint;

static const size_t type_sizes_x86[5] = { 
    4, // int 
    1, // bool
    1, // char
    4, // uint
    4  // pointer
};

static const size_t *type_sizes = NULL;

static inline Type *type_table_add(Type *type)
{
    size_t index = str_hash(type->name) & (TYPE_TABLE_SIZE - 1); 

    Type *t = type_get(type->name);
    if (t == NULL) {
        type->next = type_table[index];
        type_table[index] = type;

        return type;
    }

    if (t->is_defined)
        return NULL;

    free(t->name);
    *t = *type;
    free(type);
    return t;
}

#define TYPE_PRIM_INIT(_v, _n, _t)      \
    do {                                \
        (_v) = calloc(1, sizeof *(_v)); \
        (_v)->name = (_n);              \
        (_v)->op = (_t);                \
        (_v)->size = type_sizes[(_t)];  \
        (_v)->align = type_sizes[(_t)]; \
        (_v)->is_defined = true;        \
        type_table_add((_v));           \
    } while(0) 

void type_init()
{
    // TODO: Change this for a specific architecture in runtime
    type_sizes = type_sizes_x86;

    TYPE_PRIM_INIT(type_int, token_strings[TT_INT], TO_INT);
    TYPE_PRIM_INIT(type_bool, token_strings[TT_BOOL], TO_BOOL);
    TYPE_PRIM_INIT(type_char, token_strings[TT_CHAR], TO_CHAR);
    TYPE_PRIM_INIT(type_uint, token_strings[TT_UINT], TO_UINT);
}

void type_deinit()
{
    for (size_t i = 0; i < TYPE_TABLE_SIZE; i++) {
        Type *curr = type_table[i];
        while (curr != NULL) {
            Type *next = curr->next; 
            for (size_t j = 0; j < curr->fields_count; j++) 
                free(curr->fields[j].name);
            free(curr);

            curr = next;
        }
    }
}

Type *type_add(char *name)
{
    Type *type = calloc(1, sizeof *type);
    type->name = name;

    return type_table_add(type);
}

Type *type_get(char *name)
{
    size_t index = str_hash(name) & (TYPE_TABLE_SIZE - 1); 
    for (Type *curr = type_table[index]; curr != NULL; curr = curr->next) {
        if (curr->name == name)
            return curr;
    }

    return NULL;
}

Type *type_pointer(char *name, Type *child)
{
    Type *type = calloc(1, sizeof *type);
    type->name = name;
    type->child = child;
    type->op = TO_POINTER;
    type->size = type_sizes[TO_POINTER];
    type->align = type_sizes[TO_POINTER];
    type->is_defined = true;

    return type_table_add(type);
}

Type *type_array(char *name, Type *child, size_t elements)
{
    Type *type = calloc(1, sizeof *type);
    type->name = name;
    type->child = child;
    type->op = TO_ARRAY;
    type->size = child->size * elements;
    type->align = child->align;
    type->is_defined = true;

    return type_table_add(type);
}

Type *type_struct(char *name, Field *fields, size_t fields_count)
{
    Type *type = calloc(1, sizeof *type);
    type->name = name;
    type->op = TO_STRUCT;
    type->is_defined = true;
    type->fields = fields;
    type->fields_count = fields_count;

    size_t max_field_align = 1;

    size_t offset = 0;
    for (size_t i = 0; i < fields_count; i++) {
        size_t mod = offset % fields[i].type->align;

        if (mod != 0) 
            offset += fields[i].type->align - mod;

        fields[i].offset = offset;
        offset += fields[i].type->size;

        if (fields[i].type->align > max_field_align)
            max_field_align = fields[i].type->align;
    }

    size_t mod = offset % max_field_align;
    if (mod != 0) 
        offset += max_field_align - mod;

    type->size = offset;
    type->align = max_field_align;

    return type_table_add(type);
}
