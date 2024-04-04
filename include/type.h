#ifndef C0_TYPE_H
#define C0_TYPE_H

typedef enum TypeOp {
    TO_INT = 0,
    TO_BOOL,
    TO_CHAR,
    TO_UINT,
    TO_POINTER,

    TO_ARRAY,
    TO_STRUCT,
    TO_FUNCTION
} TypeOp;

typedef struct Type Type;
typedef struct Field Field;

struct Field {
    char *name;
    Type *type;
    size_t offset;
};

struct Type {
    TypeOp op;
    Type *child;
    size_t size;
    size_t align;

    size_t count; // Fields count for structs and args count for functions
    union { 
        Field *fields;
        Type **args;  
    } s;
};

extern Type *type_int;
extern Type *type_bool;
extern Type *type_char;
extern Type *type_uint;

void type_init();
void type_deinit();

Type *type_pointer(Type *child);
Type *type_array(Type *child, size_t elements);
Type *type_struct(Field *fields, size_t fields_num);
Type *type_function(Type *return_type, Type **args, size_t args_count);

#endif
