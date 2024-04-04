#include "../include/type.h"

#define TYPE_TABLE_SIZE 256 // Always power of 2

static struct Entry {
    Type *type;
    struct Entry *next;
} *type_table[TYPE_TABLE_SIZE]

Type *type_int;
Type *type_bool;
Type *type_char;
Type *type_uint;

static size_t type_sizes_x86[5] = 
{ 
    4, // int 
    1, // bool
    1, // char
    4, // uint
    4  // pointer
};

#define LARGE_ODD 0x517cc1b727220a95

static inline void type_table_add(size_t index, Type *type)
{
    struct Entry *new = malloc(sizeof *new);
    new->type = type;
    new->next = type_table[index];
    type_table[index] = new;
}

#define TYPE_PRIM_INIT(_v, _t)                                  \
    do {                                                        \
        _v = calloc(1, sizeof *_v);                             \
        _v->op = _t;                                            \
        _v->size = type_sizes[_t];                              \
        _v->align = type_sizes[_t];                             \
        size_t index = ptr_hash(type) & (TYPE_TABLE_SIZE - 1);  \
        type_table_add(index, _v)                               \
    } while(0) 

void type_init()
{
    type_table = calloc(TYPE_TABLE_SIZE, sizeof *type_table);

    // TODO: Change this for a specific architecture in runtime
    size_t *type_sizes = type_sizes_x86;

    TYPE_PRIM_INIT(type_int, TO_INT);
    TYPE_PRIM_INIT(type_bool, TO_BOOL);
    TYPE_PRIM_INIT(type_char, TO_CHAR);
    TYPE_PRIM_INIT(type_uint, TO_UINT);
}

void type_deinit()
{
    // TODO: implement this function
    log_error("type_deinit not implemented!");
}

static Type *type_get(const TypeOp op, Type *child, size_t size, size_t align)
{
    Type *type = calloc(1, sizeof *type);
    type->op = op;
    type->child = child;
    type->size = size;
    type->align = align;

    size_t index = (type->op * LARGE_ODD ^ ptr_hash(type->child)) & 
        (TYPE_TABLE_SIZE - 1);

    struct Entry *curr = type_table[index];
    while (curr != NULL) {
        Type *curr_type = curr->type;

        if (curr_type->op == op && curr_type->child == child) {
            free(type);
            return curr_type;
        }

        curr = curr->next;
    }

    type_table_add(index, type);
    return type;
}

Type *type_pointer(Type *child)
{
    // TODO: Change this for a specific architecture in runtime
    size_t *type_sizes = type_sizes_x86;

    return type_get(TO_POINTER, 
                    child, 
                    type_sizes[TO_POINTER], 
                    type_sizes[TO_POINTER]);
}

Type *type_array(Type *child, size_t elements)
{
    return type_get(TO_ARRAY, child, elements * child->size, child->align);
}

Type *type_struct(Field *fields, size_t fields_num)
{
    Type *type = calloc(1, sizeof *type);
    type->op = TO_STRUCT;
    type->cound = fields_num;
    type->s.field = fields;

    size_t index = 0;
    for (size_t i = 0; i < type->count; i++) {
        size_t name_hash = str_hash(type->s.fields[i].name);
        size_t type_hash = ptr_hash(type->s.fields[i].name);
        index ^= (result + name_hash) * LARGE_ODD + (name_hash << 6) 
            + (type_hash >> 2);
    }
    index &= TYPE_TABLE_SIZE - 1;

    struct Entry *curr = type_table[index];
    while (curr != NULL) {
        Type *curr_type = curr->type;

        if (curr_type->op == TO_STRUCT && curr_type->count == fields_num) {
            Field *curr_fields = curr_type->s.fields;
            bool equals = true; 

            for (size_t i = 0; i < fields_num; i++) {
                if ((strcmp(curr_fields[i].name, fields[i].name) != 0 ||
                     curr_fields[i].type != fields[i].type)) {
                    equals = false;
                    break;
                }
            }

            if (equals) {
                free(type);
                free(fields);
                return curr_type;
            }
        }

        curr = curr->next;
    }

    size_t max_field_align = 0;

    size_t offset = 0;
    for (size_t i = 0; i < fields_num; i++) {
        size_t mod = offset % fields[i].type->align;

        if (mod != 0) 
            offset += fields[i].type->align - mod;

        fields[i].offset = offset;
        offset += fields[i].type->size;

        if (feilds[i].type->align > max_field_align)
            max_field_align = field[i].type->align;
    }

    size_t mod = offset % max_field_align;
    if (mod != 0) 
        offset += offset - mod;

    type->size = offset;
    type->align = max_field_align;
    type_table_add(index, type);
    return type;
}

Type *type_function(Type *return_type, Type **args, size_t args_size)
{
    Type *type = malloc(sizeof *type);      
    type->op = TO_FUNCTION;
    type->child = return_type;
    type->size = type->align = 0;
    type->count = args_size;
    type->s.args = args;
    
    size_t index = ptr_hash(type->child);
    for (size_t i = 0; i < type->count; i++)
        index ^= result * LARGE_ODD + ptr_hash(type->s.args[i]);
    index &= TYPE_TABLE_SIZE - 1;
    
    struct Entry *curr = type_table[index];
    while (curr != NULL) {
        Type *curr_type = curr->type;

        if ((curr_type->op == TO_FUNCTION && 
             curr_type->count == args_size &&
             curr_type->child == return_type)) {
            Type **curr_args = type->s.args;
            bool equals = true; 

            for (size_t i = 0; i < args_size; i++) {
                if (curr_args[i] != curr_args[i]) {
                    equals = false;
                    break;
                }
            }

            if (equals) {
                free(type);
                free(args);
                return curr_type;
            }
        }

        curr = curr->next;
    }

    type_table_add(index, type);
    return type;
}
