#ifndef C0_TYPE_H
#define C0_TYPE_H

#include "./utils.h"

typedef enum TypeOp {
    TO_INT = 0,
    TO_BOOL,
    TO_CHAR,
    TO_UINT,
    TO_POINTER,
    TO_ARRAY,
    TO_STRUCT
} TypeOp;

typedef struct Type Type;
typedef struct Field Field;

struct Field {
    char *name;
    Type *type;
    size_t offset;
};

struct Type {
    char *name;
    Type *child;
    size_t size;
    size_t align;
    TypeOp op;
    bool is_defined;

    size_t fields_count;
    Field *fields;

    Type *next;
};

extern Type *type_int;
extern Type *type_bool;
extern Type *type_char;
extern Type *type_uint;

void type_init();
void type_deinit();

Type *type_add(char *name);
Type *type_get(char *name);

Type *type_pointer(char *name, Type *child);
Type *type_array(char *name, Type *child, size_t elements);
Type *type_struct(char *name, Field *fields, size_t fields_count);

#endif
