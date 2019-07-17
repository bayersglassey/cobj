#ifndef _COBJ_H_
#define _COBJ_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/* #define COBJ_DEBUG_TOKENS */


/* Users are free to use the unmasked bits of obj->tag
however they wish */
#define OBJ_TYPE_MASK (16-1)

#define OBJ_TYPE(obj) ((obj)[0].tag & OBJ_TYPE_MASK)
#define OBJ_BOOL(obj) (bool)(obj)[0].u.i
#define OBJ_INT(obj) (obj)[0].u.i
#define OBJ_SYM(obj) (obj)[0].u.y
#define OBJ_STRING(obj) (obj)[0].u.s
#define OBJ_DICT(obj) (obj)[0].u.d
#define OBJ_HEAD(obj) (obj)[0].u.o
#define OBJ_TAIL(obj) (obj)[1].u.o
#define OBJ_CONTENTS(obj) (obj)[0].u.o
#define OBJ_RESOLVE(obj) obj_resolve(obj)

#define OBJ_LIST_GET(obj, sym) obj_list_get(obj, sym)
#define OBJ_LIST_IGET(obj, i) obj_list_iget(obj, i)
#define OBJ_LIST_LEN(obj) obj_list_len(obj)
#define OBJ_ARRAY_IGET(obj, i) ((obj) + 1 + (i))
#define OBJ_ARRAY_LEN(obj) (obj)[0].u.i
#define OBJ_DICT_KEYS_LEN(obj) (obj)[0].u.d->n_entries
#define OBJ_DICT_GET(obj, sym) obj_dict_get(obj, sym)
#define OBJ_STRUCT_KEYS_LEN(obj) (obj)[0].u.i
#define OBJ_STRUCT_IGET_KEY(obj, i) ((obj) + 1 + (i) * 2)
#define OBJ_STRUCT_IGET_VAL(obj, i) ((obj) + 1 + (i) * 2 + 1)
#define OBJ_STRUCT_GET(obj, sym) obj_struct_get(obj, sym)
#define OBJ_GET(obj, sym) obj_get(obj, sym)
#define OBJ_IGET(obj, i) obj_iget(obj, i)
#define OBJ_LEN(obj) obj_len(obj)

#ifndef OBJ_POOL_CHUNK_LEN
#   define OBJ_POOL_CHUNK_LEN 1024
#endif

#define OBJ_SYMTABLE_DEFAULT_SIZE 16
#define OBJ_PARSER_TOKEN_BUFFER_DEFAULT_SIZE 512
#define OBJ_DICT_DEFAULT_SIZE 16

const char ASCII_OPERATORS[] = "!$%&'*+,-./<=>?@^`|~";
const char ASCII_LOWER[] = "abcdefghijklmnopqrstuvwxyz";
const char ASCII_UPPER[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct obj obj_t;
typedef struct obj_string obj_string_t;
typedef struct obj_string_list obj_string_list_t;
typedef struct obj_sym obj_sym_t;
typedef struct obj_symtable obj_symtable_t;
typedef struct obj_dict obj_dict_t;
typedef struct obj_dict_entry obj_dict_entry_t;
typedef struct obj_dict_list obj_dict_list_t;
typedef struct obj_pool obj_pool_t;
typedef struct obj_pool_chunk obj_pool_chunk_t;
typedef struct obj_parser obj_parser_t;
typedef struct obj_parser_stack obj_parser_stack_t;

enum {
    OBJ_TYPE_NULL,
    OBJ_TYPE_BOOL,
    OBJ_TYPE_INT,
    OBJ_TYPE_SYM,
    OBJ_TYPE_STR,
    OBJ_TYPE_NIL,
    OBJ_TYPE_CELL,
    OBJ_TYPE_TAIL,
    OBJ_TYPE_ARRAY,
    OBJ_TYPE_DICT,
    OBJ_TYPE_STRUCT,
    OBJ_TYPE_BOX,
    OBJ_TYPES,
    OBJ_TYPE_UNDEFINED=-1
};
const char *obj_type_msg(int type){
    static const char *msgs[OBJ_TYPES] = {
        "null", "bool", "int", "sym", "str", "nil", "cell", "tail",
        "array", "dict", "struct", "box"
    };
    if(type == OBJ_TYPE_UNDEFINED)return "undefined";
    if(type < 0 || type >= OBJ_TYPES)return "unknown";
    return msgs[type];
}

enum {
    OBJ_TOKEN_TYPE_INVALID,
    OBJ_TOKEN_TYPE_EOF,
    OBJ_TOKEN_TYPE_WHITESPACE,
    OBJ_TOKEN_TYPE_COMMENT,
    OBJ_TOKEN_TYPE_NEWLINE,
    OBJ_TOKEN_TYPE_INT,
    OBJ_TOKEN_TYPE_NAME,
    OBJ_TOKEN_TYPE_OPER,
    OBJ_TOKEN_TYPE_LONGSYM,
    OBJ_TOKEN_TYPE_TYPECAST,
    OBJ_TOKEN_TYPE_STRING,
    OBJ_TOKEN_TYPE_LINESTRING,
    OBJ_TOKEN_TYPE_LPAREN,
    OBJ_TOKEN_TYPE_RPAREN,
    OBJ_TOKEN_TYPE_COLON,
    OBJ_TOKEN_TYPES
};
const char *obj_token_type_msg(int type){
    static const char *msgs[OBJ_TOKEN_TYPES] = {
        "invalid", "eof", "whitespace", "comment", "newline",
        "int", "name", "oper", "longsym", "typecast", "string",
        "linestring", "lparen", "rparen", "colon"
    };
    if(type < 0 || type >= OBJ_TOKEN_TYPES)return "unknown";
    return msgs[type];
}

enum {
    OBJ_SYMBOL_TYPE_NAME,
    OBJ_SYMBOL_TYPE_OPER,
    OBJ_SYMBOL_TYPE_LONGSYM,
};

struct obj {
    int tag;
    union {
        int i;
        obj_sym_t *y;
        obj_string_t *s;
        obj_t *o;
        obj_dict_t *d;
    } u;
};

struct obj_string {
    size_t len;
    char *data;
};

struct obj_string_list {
    obj_string_list_t *next;
    obj_string_t string;
};

struct obj_sym {
    size_t hash;
    obj_string_t string;
};

struct obj_symtable {
    obj_sym_t **syms;
    size_t syms_len;
    size_t n_syms;
        /* syms_len - size of allocated space */
        /* n_syms - number of symbols stored in table */
        /* n_syms <= syms_len */
};

struct obj_dict {
    obj_dict_entry_t *entries;
    size_t entries_len;
    size_t n_entries;
        /* entries_len - size of allocated space */
        /* n_entries - number of entries stored in dict */
        /* n_entries <= entries_len */
};

struct obj_dict_entry {
    obj_sym_t *sym;
    void *value;
};

struct obj_dict_list {
    obj_dict_list_t *next;
    obj_dict_t dict;
};

struct obj_pool {
    obj_symtable_t *symtable;
    obj_pool_chunk_t *chunk_list;
    obj_string_list_t *string_list;
    obj_dict_list_t *dict_list;

    /* Unique objects, doesn't make sense to keep allocating them */
    obj_t null;
    obj_t nil;
    obj_t T;
    obj_t F;
};

struct obj_pool_chunk {
    obj_pool_chunk_t *next;
    obj_t objs[OBJ_POOL_CHUNK_LEN];
    size_t len;
};

struct obj_parser {
    bool use_extended_types; /* array, dict */
    obj_pool_t *pool;
    const char *filename;
    const char *data;
    size_t data_len;

    size_t pos;
    size_t row;
    size_t col;

    char *token_buffer;
    size_t token_buffer_len;

    const char *token;
    int token_type;
    size_t token_pos;
    size_t token_row;
    size_t token_col;
    size_t token_len;
    size_t line_col;
    bool line_col_is_set;

    obj_parser_stack_t *stack;
    obj_parser_stack_t *free_stack;
        /* free_stack is a linked list of preallocated stack
        entries.
        So when we pop from this->stack, instead of freeing,
        we push onto this->free_stack.
        Then when we push to this->stack, we pop from
        this->free_stack if available, only otherwise do we
        malloc. */
};

struct obj_parser_stack {
    obj_parser_stack_t *next;
    int token_type;
    size_t token_row;
    size_t line_col;

    obj_t **tail;
        /* The tail ptr of last node in list we were working on
        when we encountered this COLON or LPAREN token.
        At that time, we pushed this stack entry, and started
        working on a fresh list.
        When we encounter the closing token (e.g. RPAREN), we
        pop this stack entry and continue working on the old
        list. */
};



/*************
* prototypes *
*************/

static void obj_fprint(obj_t *obj, FILE *file, int depth);
void obj_init_null(obj_t *obj);
void obj_init_bool(obj_t *obj, bool b);
void obj_init_nil(obj_t *obj);


/************
* utilities *
************/

static void _print_tabs(FILE *file, int depth){
    for(int i = 0; i < depth; i++)putc(' ', file);
}

static int size_to_int(size_t size, int max){
    /* For doing printf("%.*s", size, text) where size is a size_t, and
    needs to be converted to int.
    If max >= 0, the returned value is guaranteed to be <= max.
    If max < 0, it's ignored. */
    int len = (int)size;
    if(len < 0)len = 0;
    else if(max >= 0 && len > max)len = max;
    return len;
}

int obj_hash(const char *s, int len){
    /* The classic hash by Bernstein */
    int hash = 5381;
    for(int i = 0; i < len; i++){
        hash = hash * 33 + s[i];
    }
    return hash;
}

int obj_symbol_type(const char *token, size_t token_len){
    if(!token_len)return OBJ_SYMBOL_TYPE_LONGSYM;

    char c = token[0];
    if(c == '_' || strchr(ASCII_LOWER, c) || strchr(ASCII_UPPER, c)){
        for(size_t i = 1; i < token_len; i++){
            c = token[i];
            if(!(
                c == '_' ||
                (c >= '0' && c <= '9') ||
                strchr(ASCII_LOWER, c) ||
                strchr(ASCII_UPPER, c)
            ))return OBJ_SYMBOL_TYPE_LONGSYM;
        }
        return OBJ_SYMBOL_TYPE_NAME;
    }else if(strchr(ASCII_OPERATORS, c)){
        for(size_t i = 1; i < token_len; i++){
            c = token[i];
            if(!strchr(ASCII_OPERATORS, c))return OBJ_SYMBOL_TYPE_LONGSYM;
        }
        return OBJ_SYMBOL_TYPE_OPER;
    }

    return OBJ_SYMBOL_TYPE_LONGSYM;
}


/**********************
* obj_string, obj_sym *
**********************/

obj_string_t *obj_string_init(obj_string_t *string, size_t len){
    string->len = 0;
    string->data = malloc(len);
    if(!string->data){
        fprintf(stderr,
            "%s: Couldn't allocate %zu bytes of string data. ",
                __func__, len);
        perror("malloc");
        return NULL;
    }
    string->len = len;
    return string;
}

void obj_string_cleanup(obj_string_t *string){
    free(string->data);
}

bool obj_string_eq_raw(obj_string_t *string,
    const char *text, size_t text_len
){
    return string->len == text_len &&
        !strncmp(string->data, text, text_len);
}

bool obj_string_eq(obj_string_t *string1, obj_string_t *string2){
    if(string1 == string2)return true;
    if(string1 == NULL || string2 == NULL)return string1 == string2;
    return obj_string_eq_raw(string1, string2->data, string2->len);
}

void obj_string_fprint_raw(
    obj_string_t *s, FILE *file, char lc, char rc,
    const char *escaped_chars
){
    if(lc)putc(lc, file);
    if(!escaped_chars){
        int len = size_to_int(s->len, -1);
        fprintf(file, "%.*s", len, s->data);
    }else{
        for(size_t i = 0; i < s->len; i++){
            char c = s->data[i];
            if(strchr(escaped_chars, c)){
                putc('\\', file);
                if(c == '\n')c = 'n';
            }
            putc(c, file);
        }
    }
    if(rc)putc(rc, file);
}

void obj_string_fprint(obj_string_t *s, FILE *file){
    obj_string_fprint_raw(s, file, '"', '"', "\\\n\"");
}

void obj_sym_fprint(obj_sym_t *sym, FILE *file){
    int type = obj_symbol_type(sym->string.data, sym->string.len);
    char lc = 0;
    char rc = 0;
    if(type == OBJ_SYMBOL_TYPE_LONGSYM){
        lc = '[';
        rc = ']';
    }
    obj_string_fprint_raw(&sym->string, file, lc, rc, "\\\n[]");
}


/***************
* obj_symtable *
***************/

void obj_symtable_init(obj_symtable_t *table){
    memset(table, 0, sizeof(*table));
}

void obj_symtable_cleanup(obj_symtable_t *table){
    for(size_t i = 0; i < table->syms_len; i++){
        obj_sym_t *sym = table->syms[i];
        if(!sym)continue;
        obj_string_cleanup(&sym->string);
        free(sym);
    }
    free(table->syms);
}

void obj_symtable_dump(obj_symtable_t *table, FILE *file){
    fprintf(file, "SYMTABLE %p (%zu/%zu):\n",
        table, table->n_syms, table->syms_len);
    for(size_t i = 0; i < table->syms_len; i++){
        obj_sym_t *sym = table->syms[i];
        fprintf(file, "  SYM %p", sym);
        if(!sym){
            fprintf(file, "\n");
            continue;
        }
        fprintf(file, ": ");
        obj_sym_fprint(sym, file);
        putc('\n', file);
    }
}

void obj_symtable_errmsg(obj_symtable_t *table, const char *funcname){
    fprintf(stderr, "%s [%zu/%zu]: ",
        funcname, table->n_syms, table->syms_len);
}

obj_sym_t **obj_symtable_find_free_slot(obj_symtable_t *table, size_t hash){
    /* Finds next free slot (pointer into table->syms) for given hash.
    Returns NULL if no free slots. */
    size_t mask = table->syms_len - 1;
    size_t i0 = hash & mask;
    size_t i = i0;
    do {
        obj_sym_t *sym = table->syms[i];
        if(sym == NULL){
            return &table->syms[i];
        }
        i = (i + 1) & mask;
    }while(i != i0);
    return NULL;
}

obj_sym_t *obj_symtable_add_sym(obj_symtable_t *table, obj_sym_t *sym){
    /* Adds the given sym to the table. Returns sym if there was space,
    NULL otherwise. */
    obj_sym_t **slot_ptr = obj_symtable_find_free_slot(table, sym->hash);
    if(!slot_ptr)return NULL;
    *slot_ptr = sym;
    return sym;
}

int obj_symtable_grow(obj_symtable_t *table){
    obj_sym_t **old_syms = table->syms;
    size_t old_syms_len = table->syms_len;

    size_t syms_len = old_syms_len? old_syms_len * 2:
        OBJ_SYMTABLE_DEFAULT_SIZE;
    obj_sym_t **syms = calloc(sizeof(*syms), syms_len);
    if(!syms){
        obj_symtable_errmsg(table, __func__);
        fprintf(stderr,
            "Trying to allocate new syms of size %zu. ", syms_len);
        perror("calloc");
        return 1;
    }

    table->syms = syms;
    table->syms_len = syms_len;

    for(size_t i = 0; i < old_syms_len; i++){
        obj_sym_t *old_sym = old_syms[i];
        if(!old_sym)continue;
        obj_sym_t *new_sym = obj_symtable_add_sym(table, old_sym);
        if(!new_sym){
            /* Shouldn't be possible for space not to be found for
            sym, since we just grew the table!
            But might as well check for null pointer anyway. */
            obj_symtable_errmsg(table, __func__);
            fprintf(stderr, "Couldn't move sym %zu/%zu\n",
                i, old_syms_len);
            table->syms = old_syms;
            table->syms_len = old_syms_len;
            free(syms);
            return 1;
        }
    }

    free(old_syms);
    return 0;
}

obj_sym_t *obj_symtable_create_sym_raw(
    obj_symtable_t *table, const char *text, size_t text_len, size_t hash
){
    /* Allocates & adds a new sym to the table with given text etc */

    /* We grow table once its 3/4 full */
    if(table->n_syms >= table->syms_len / 4 * 3){
        if(obj_symtable_grow(table))return NULL;
    }

    obj_sym_t *sym = calloc(sizeof(*sym), 1);
    if(!sym){
        obj_symtable_errmsg(table, __func__);
        fprintf(stderr, "While getting sym for \"%.*s\": ",
            size_to_int(text_len, 256), text);
        fprintf(stderr, "Couldn't allocate sym. ");
        perror("calloc");
        return NULL;
    }
    obj_string_t *string = &sym->string;
    if(!obj_string_init(string, text_len)){
        obj_symtable_errmsg(table, __func__);
        fprintf(stderr, "While getting sym for \"%.*s\": ",
            size_to_int(text_len, 256), text);
        fprintf(stderr, "Couldn't initialize string.\n");
        free(sym);
        return NULL;
    }
    memcpy(string->data, text, text_len);
    sym->hash = hash;
    if(!obj_symtable_add_sym(table, sym))return NULL;

    table->n_syms++;
    return sym;
}

obj_sym_t *obj_symtable_get_sym_raw(
    obj_symtable_t *table, const char *text, size_t text_len
){
    /* Gets (first creating, if necessary) the sym for given text.
    When creating sym, text is copied, so caller remains ownership
    of (and responsibility for freeing) the original. */

    size_t hash = obj_hash(text, text_len);

    if(!table->syms_len){
        /* If table is empty, skip the search for existing sym & just
        create a new one */
        return obj_symtable_create_sym_raw(table, text, text_len, hash);
    }

    size_t mask = table->syms_len - 1;
    size_t i0 = hash & mask;
    size_t i = i0;
    do {
        obj_sym_t *sym = table->syms[i];
        if(sym != NULL && sym->hash == hash){
            if(obj_string_eq_raw(&sym->string, text, text_len)){
                /* Found sym matching given text! */
                return sym;
            }
        }
        i = (i + 1) & mask;
    }while(i != i0);

    /* Found no sym matching given text, so we'll add a new one. */
    return obj_symtable_create_sym_raw(table, text, text_len, hash);
}

obj_sym_t *obj_symtable_get_sym(obj_symtable_t *table, const char *text){
    return obj_symtable_get_sym_raw(table, text, strlen(text));
}


/***********
* obj_dict *
***********/

void obj_dict_init(obj_dict_t *dict){
    memset(dict, 0, sizeof(*dict));
}

void obj_dict_cleanup(obj_dict_t *dict){
    free(dict->entries);
}

void obj_dict_dump(obj_dict_t *dict, FILE *file){
    fprintf(file, "DICT %p (%zu/%zu):\n",
        dict, dict->n_entries, dict->entries_len);
    for(size_t i = 0; i < dict->entries_len; i++){
        obj_dict_entry_t *entry = &dict->entries[i];
        obj_sym_t *sym = entry->sym;
        fprintf(file, "  SYM %p", sym);
        if(!sym){
            fprintf(file, "\n");
            continue;
        }
        fprintf(file, ": ");
        obj_sym_fprint(sym, file);
        putc('\n', file);
        fprintf(file, "    -> %p\n", entry->value);
    }
}

void obj_dict_fprint(obj_dict_t *dict, FILE *file, int depth){
    for(size_t i = 0; i < dict->entries_len; i++){
        obj_dict_entry_t *entry = &dict->entries[i];
        if(!entry->sym)continue;

        putc('\n', file);
        _print_tabs(file, depth);

        obj_sym_fprint(entry->sym, file);

        putc(' ', file);
        obj_fprint((obj_t*)entry->value, file, depth);
    }
}

void obj_dict_errmsg(obj_dict_t *dict, const char *funcname){
    fprintf(stderr, "%s [%zu/%zu]: ",
        funcname, dict->n_entries, dict->entries_len);
}

obj_dict_entry_t *obj_dict_find_free_entry(obj_dict_t *dict, size_t hash){
    /* Finds next free (sym == NULL) entry for given hash.
    Returns NULL if no free entries. */
    size_t mask = dict->entries_len - 1;
    size_t i0 = hash & mask;
    size_t i = i0;
    do {
        obj_sym_t *sym = dict->entries[i].sym;
        if(sym == NULL){
            return &dict->entries[i];
        }
        i = (i + 1) & mask;
    }while(i != i0);
    return NULL;
}

obj_dict_entry_t *obj_dict_add_entry(obj_dict_t *dict, obj_sym_t *sym, void *value){
    /* Adds an entry to the dict. Returns entry if there was space,
    NULL otherwise. */
    obj_dict_entry_t *entry = obj_dict_find_free_entry(dict, sym->hash);
    if(!entry)return NULL;
    entry->sym = sym;
    entry->value = value;
    return entry;
}

int obj_dict_grow(obj_dict_t *dict){
    obj_dict_entry_t *old_entries = dict->entries;
    size_t old_entries_len = dict->entries_len;

    size_t entries_len = old_entries_len? old_entries_len * 2:
        OBJ_DICT_DEFAULT_SIZE;
    obj_dict_entry_t *entries = calloc(sizeof(*entries), entries_len);
    if(!entries){
        obj_dict_errmsg(dict, __func__);
        fprintf(stderr,
            "Trying to allocate new entries of size %zu. ", entries_len);
        perror("calloc");
        return 1;
    }

    dict->entries = entries;
    dict->entries_len = entries_len;

    for(size_t i = 0; i < old_entries_len; i++){
        obj_dict_entry_t *old_entry = &old_entries[i];
        if(!old_entry->sym)continue;
        obj_dict_entry_t *new_entry = obj_dict_add_entry(dict,
            old_entry->sym, old_entry->value);
        if(!new_entry){
            /* Shouldn't be possible for space not to be found for
            entry, since we just grew the dict!
            But might as well check for null pointer anyway. */
            obj_dict_errmsg(dict, __func__);
            fprintf(stderr, "Couldn't move entry %zu/%zu\n",
                i, old_entries_len);
            dict->entries = old_entries;
            dict->entries_len = old_entries_len;
            free(entries);
            return 1;
        }
    }

    free(old_entries);
    return 0;
}

obj_dict_entry_t *obj_dict_get_entry(obj_dict_t *dict, obj_sym_t *sym){
    /* Gets the entry for given sym, or NULL if not found. */

    if(!dict->entries_len){
        /* If dict is empty, return NULL immediately.
        Otherwise, mask will be borked due to underflow. */
        return NULL;
    }

    size_t mask = dict->entries_len - 1;
    size_t i0 = sym->hash & mask;
    size_t i = i0;
    do {
        obj_dict_entry_t *entry = &dict->entries[i];
        if(entry->sym == sym)return entry;
        i = (i + 1) & mask;
    }while(i != i0);
    return NULL;
}

void *obj_dict_get(obj_dict_t *dict, obj_sym_t *sym){
    /* Gets the value for given sym, or NULL if not found. */
    obj_dict_entry_t *entry = obj_dict_get_entry(dict, sym);
    if(!entry)return NULL;
    return entry->value;
}

obj_dict_entry_t *obj_dict_set(obj_dict_t *dict, obj_sym_t *sym, void *value){
    /* Adds (or updates existing) entry with given sym and value. */

    obj_dict_entry_t *entry = obj_dict_get_entry(dict, sym);
    if(entry){
        entry->value = value;
        return entry;
    }

    /* We grow dict once its 3/4 full */
    if(dict->n_entries >= dict->entries_len / 4 * 3){
        if(obj_dict_grow(dict))return NULL;
    }

    entry = obj_dict_add_entry(dict, sym, value);
    if(!entry)return NULL;
    dict->n_entries++;
    return entry;
}


/***********
* obj_pool *
***********/

void obj_pool_init(obj_pool_t *pool, obj_symtable_t *symtable){
    memset(pool, 0, sizeof(*pool));
    pool->symtable = symtable;

    obj_init_null(&pool->null);
    obj_init_nil(&pool->nil);
    obj_init_bool(&pool->T, true);
    obj_init_bool(&pool->F, false);
}

void obj_pool_cleanup(obj_pool_t *pool){
    for(obj_pool_chunk_t *chunk = pool->chunk_list; chunk;){
        obj_pool_chunk_t *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    for(obj_string_list_t *string_list = pool->string_list; string_list;){
        obj_string_list_t *next = string_list->next;
        obj_string_cleanup(&string_list->string);
        free(string_list);
        string_list = next;
    }
    for(obj_dict_list_t *dict_list = pool->dict_list; dict_list;){
        obj_dict_list_t *next = dict_list->next;
        obj_dict_cleanup(&dict_list->dict);
        free(dict_list);
        dict_list = next;
    }
}

void obj_pool_errmsg(obj_pool_t *pool, const char *funcname){
    fprintf(stderr, "%s: ", funcname);
}

void obj_pool_dump(obj_pool_t *pool, FILE *file){
    fprintf(file, "OBJ POOL %p:\n", pool);

    fprintf(file, "  CHUNKS:\n");
    for(
        obj_pool_chunk_t *chunk = pool->chunk_list;
        chunk; chunk = chunk->next
    ){
        fprintf(file, "    CHUNK %p: %zu/%zu\n",
            chunk, chunk->len, (size_t)OBJ_POOL_CHUNK_LEN);
    }

    fprintf(file, "  STRINGS:\n");
    for(obj_string_list_t *string_list = pool->string_list;
        string_list; string_list = string_list->next
    ){
        obj_string_t *string = &string_list->string;
        fprintf(file, "    STRING %p (%zu): \"%.*s\"%s\n",
            string, string->len, size_to_int(string->len, 40),
            string->data, string->len > 40? "...": "");
    }

    fprintf(file, "  DICTS:\n");
    for(obj_dict_list_t *dict_list = pool->dict_list;
        dict_list; dict_list = dict_list->next
    ){
        obj_dict_t *dict = &dict_list->dict;
        fprintf(file, "    DICT %p (%zu/%zu)\n",
            dict, dict->n_entries, dict->entries_len);
    }
}

obj_string_t *obj_pool_string_alloc(obj_pool_t *pool, size_t len){

    /* add new linked list entry */
    obj_string_list_t *string_list = calloc(sizeof(*string_list), 1);
    if(!string_list){
        fprintf(stderr, "%s: Couldn't allocate new string list node. ",
            __func__);
        perror("calloc");
        return NULL;
    }
    string_list->next = pool->string_list;
    pool->string_list = string_list;

    /* alloc new string */
    obj_string_t *string = &string_list->string;
    if(!obj_string_init(string, len)){
        fprintf(stderr, "%s: Couldn't initialize string.\n", __func__);
        free(string_list);
        return NULL;
    }

    return string;
}

obj_string_t *obj_pool_string_add_raw(obj_pool_t *pool, const char *data, size_t len){
    /* "raw" meaning length is specified, instead of NUL-terminated */
    obj_string_t *string = obj_pool_string_alloc(pool, len);
    if(!string)return NULL;
    memcpy(string->data, data, len);
    return string;
}

obj_string_t *obj_pool_string_add(obj_pool_t *pool, const char *data){
    return obj_pool_string_add_raw(pool, data, strlen(data));
}

obj_dict_t *obj_pool_dict_alloc(obj_pool_t *pool){

    /* add new linked list entry */
    obj_dict_list_t *dict_list = calloc(sizeof(*dict_list), 1);
    if(!dict_list){
        fprintf(stderr, "%s: Couldn't allocate new dict list node. ",
            __func__);
        perror("calloc");
        return NULL;
    }
    dict_list->next = pool->dict_list;
    pool->dict_list = dict_list;

    /* initialize dict */
    obj_dict_t *dict = &dict_list->dict;
    obj_dict_init(dict);

    return dict;
}

obj_t *obj_pool_objs_alloc(obj_pool_t *pool, size_t n_objs){
    obj_pool_chunk_t *chunk = pool->chunk_list;
    if(!chunk || chunk->len + n_objs >= OBJ_POOL_CHUNK_LEN){
        obj_pool_chunk_t *new_chunk = calloc(sizeof(*new_chunk), 1);
        if(new_chunk == NULL){
            obj_pool_errmsg(pool, __func__);
            perror("calloc");
            return NULL;
        }
        new_chunk->next = chunk;
        chunk = new_chunk;
        pool->chunk_list = chunk;
    }

    obj_t *obj = &chunk->objs[chunk->len];
    chunk->len += n_objs;
    memset(obj, 0, sizeof(*obj));
    return obj;
}

void obj_init_null(obj_t *obj){
    obj->tag = OBJ_TYPE_NULL;
}

void obj_init_bool(obj_t *obj, bool b){
    obj->tag = OBJ_TYPE_BOOL;
    OBJ_INT(obj) = b;
}

void obj_init_int(obj_t *obj, int i){
    obj->tag = OBJ_TYPE_INT;
    OBJ_INT(obj) = i;
}

void obj_init_sym(obj_t *obj, obj_sym_t *sym){
    obj->tag = OBJ_TYPE_SYM;
    OBJ_SYM(obj) = sym;
}

void obj_init_str(obj_t *obj, obj_string_t *string){
    obj->tag = OBJ_TYPE_STR;
    OBJ_STRING(obj) = string;
}

void obj_init_nil(obj_t *obj){
    obj->tag = OBJ_TYPE_NIL;
}

void obj_init_dict(obj_t *obj, obj_dict_t *dict){
    obj->tag = OBJ_TYPE_DICT;
    OBJ_DICT(obj) = dict;
}

void obj_init_box(obj_t *obj, obj_t *contents){
    obj->tag = OBJ_TYPE_BOX;
    OBJ_CONTENTS(obj) = contents;
}

obj_t *obj_pool_add_null(obj_pool_t *pool){
    return &pool->null;
}

obj_t *obj_pool_add_bool(obj_pool_t *pool, bool b){
    return b? &pool->T: &pool->F;
}

obj_t *obj_pool_add_int(obj_pool_t *pool, int i){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_INT;
    OBJ_INT(obj) = i;
    return obj;
}

obj_t *obj_pool_add_sym(obj_pool_t *pool, obj_sym_t *sym){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_SYM;
    OBJ_SYM(obj) = sym;
    return obj;
}

obj_t *obj_pool_add_str(obj_pool_t *pool, obj_string_t *string){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_STR;
    OBJ_STRING(obj) = string;
    return obj;
}

obj_t *obj_pool_add_nil(obj_pool_t *pool){
    return &pool->nil;
}

obj_t *obj_pool_add_cell(obj_pool_t *pool, obj_t *head, obj_t *tail){
    obj_t *obj = obj_pool_objs_alloc(pool, 2);
    if(!obj)return NULL;
    obj[0].tag = OBJ_TYPE_CELL;
    obj[1].tag = OBJ_TYPE_TAIL;
    OBJ_HEAD(obj) = head;
    OBJ_TAIL(obj) = tail;
    return obj;
}

obj_t *obj_pool_add_array(obj_pool_t *pool, int len){
    obj_t *obj = obj_pool_objs_alloc(pool, 1 + len);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_ARRAY;
    OBJ_ARRAY_LEN(obj) = len;
    for(int i = 0; i < len; i++){
        obj_init_null(OBJ_ARRAY_IGET(obj, i));
    }
    return obj;
}

obj_t *obj_pool_add_dict_raw(obj_pool_t *pool, obj_dict_t *dict){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_DICT;
    OBJ_DICT(obj) = dict;
    return obj;
}

obj_t *obj_pool_add_dict(obj_pool_t *pool){
    obj_dict_t *dict = obj_pool_dict_alloc(pool);
    if(!dict)return NULL;
    return obj_pool_add_dict_raw(pool, dict);
}

obj_t *obj_pool_add_struct(obj_pool_t *pool, int n_keys){
    obj_t *obj = obj_pool_objs_alloc(pool, 1 + n_keys * 2);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_STRUCT;
    OBJ_STRUCT_KEYS_LEN(obj) = n_keys;
    for(int i = 0; i < n_keys; i++){
        obj_init_sym(OBJ_STRUCT_IGET_KEY(obj, i), NULL);
        obj_init_null(OBJ_STRUCT_IGET_VAL(obj, i));
    }
    return obj;
}

obj_t *obj_pool_add_box(obj_pool_t *pool, obj_t *contents){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_BOX;
    OBJ_CONTENTS(obj) = contents;
    return obj;
}


/*************
* obj_parser *
*************/

void obj_parser_init(
    obj_parser_t *parser, obj_pool_t *pool,
    const char *filename, const char *data, size_t data_len
){
    memset(parser, 0, sizeof(*parser));
    /* parser->use_extended_types = true; */
    parser->pool = pool;
    parser->filename = filename;
    parser->data = data;
    parser->data_len = data_len;
}

void obj_parser_stack_cleanup(obj_parser_stack_t *stack){
    while(stack){
        obj_parser_stack_t *next = stack->next;
        free(stack);
        stack = next;
    }
}

void obj_parser_cleanup(obj_parser_t *parser){
    free(parser->token_buffer);
    obj_parser_stack_cleanup(parser->stack);
    obj_parser_stack_cleanup(parser->free_stack);
}

void obj_parser_dump(obj_parser_t *parser, FILE *file){
    fprintf(file, "OBJ PARSER %p:\n", parser);
    fprintf(file, "  TOKEN BUFFER: %p (%zu)\n",
        parser->token_buffer, parser->token_buffer_len);
    fprintf(file, "  STACK:\n");
    for(obj_parser_stack_t *stack = parser->stack;
        stack; stack = stack->next
    ){
        fprintf(file, "    ENTRY %p: row=%zu col=%zu type=%i\n", stack,
            stack->token_row, stack->line_col, stack->token_type);
    }
    fprintf(file, "  FREE-STACK:\n");
    for(obj_parser_stack_t *free_stack = parser->free_stack;
        free_stack; free_stack = free_stack->next
    ){
        fprintf(file, "    ENTRY %p\n", free_stack);
    }
}

void obj_parser_errmsg(obj_parser_t *parser, const char *funcname){
    fprintf(stderr, "%s [pos=%zu/%zu row=%zu col=%zu]: ",
        funcname, parser->token_pos, parser->data_len,
        parser->token_row+1, parser->token_col+1);
    fprintf(stderr, "Token was: \"%.*s\"\n",
        size_to_int(parser->token_len, 256), parser->token);
}

char *obj_parser_get_token_buffer(obj_parser_t *parser, size_t want_len){
    if(parser->token_buffer_len >= want_len)return parser->token_buffer;
    size_t len = parser->token_buffer_len;
    if(!len)len = OBJ_PARSER_TOKEN_BUFFER_DEFAULT_SIZE;
    while(len < want_len){
        size_t old_len = len;
        len = len / 2 * 3;
        if(len < old_len){
            /* C is... silly */
            fprintf(stderr,
                "%s: Token buffer overflow while requesting %zu bytes\n",
                __func__, want_len);
            return NULL;
        }
    }
    char *buffer = malloc(len);
    if(!buffer)return NULL;
    parser->token_buffer_len = len;
    parser->token_buffer = buffer;
    return buffer;
}

bool obj_parser_token_is_sym(obj_parser_t *parser){
    int type = parser->token_type;
    return
        type == OBJ_TOKEN_TYPE_NAME ||
        type == OBJ_TOKEN_TYPE_OPER ||
        type == OBJ_TOKEN_TYPE_LONGSYM;
}

bool obj_parser_token_is_string(obj_parser_t *parser){
    int type = parser->token_type;
    return
        type == OBJ_TOKEN_TYPE_STRING ||
        type == OBJ_TOKEN_TYPE_LINESTRING;
}

bool obj_parser_token_eq(obj_parser_t *parser, const char *text){
    size_t text_len = strlen(text);
    return parser->token_len == text_len &&
        !strncmp(parser->token, text, text_len);
}

obj_parser_stack_t *obj_parser_stack_push(obj_parser_t *parser, obj_t **tail){
    obj_parser_stack_t *stack;
    if(parser->free_stack){
        stack = parser->free_stack;
        parser->free_stack = parser->free_stack->next;
    }else{
        stack = calloc(sizeof(*stack), 1);
        if(!stack){
            obj_parser_errmsg(parser, __func__);
            perror("calloc");
            return NULL;
        }
    }

    memset(stack, 0, sizeof(*stack));
    stack->token_type = parser->token_type;
    stack->token_row = parser->token_row;
    stack->line_col = parser->line_col;
    stack->tail = tail;

    stack->next = parser->stack;
    parser->stack = stack;
    return stack;
}

obj_t **obj_parser_stack_pop(obj_parser_t *parser, obj_t **tail){
    *tail = obj_pool_add_nil(parser->pool);
    if(!*tail)return NULL;

    tail = parser->stack->tail;

    obj_parser_stack_t *next = parser->stack->next;
    parser->stack->next = parser->free_stack;
    parser->free_stack = parser->stack;
    parser->stack = next;

    return tail;
}


int obj_parser_get_token(obj_parser_t *parser){
    /* Updates parser->token, parser->token_len.
    If end of parser->data is reached (that is,
    parser->pos >= parser->data_len), then parser->token is set to NULL.
    NOTE: parser->data is not allowed to contain NUL bytes. */

#   define OBJ_PARSER_GETC() \
        if(parser->pos >= parser->data_len){ \
            c = EOF; \
        }else { \
            parser->pos++; \
            if(c == '\n'){ \
                parser->row++; \
                parser->col = 0; \
                parser->line_col = 0; \
                parser->line_col_is_set = false; \
            }else{ \
                parser->col++; \
            } \
            c = parser->pos >= parser->data_len? EOF: \
                parser->data[parser->pos]; \
        }

    /* No matter what, token starts from current location.
    So, possible "tokens" include whitespace, newlines, EOF,
    as well as regular stuff like numbers, symbols, strings,
    etc. */
    parser->token = &parser->data[parser->pos];
    parser->token_pos = parser->pos;
    parser->token_row = parser->row;
    parser->token_col = parser->col;

    int c = parser->pos >= parser->data_len? EOF:
        parser->data[parser->pos];

    if(c == EOF){
        /* EOF */
        parser->token_type = OBJ_TOKEN_TYPE_EOF;
        /* Return empty token */
    }else if(c == '\n'){
        /* Newline */
        parser->token_type = OBJ_TOKEN_TYPE_NEWLINE;
        OBJ_PARSER_GETC()
    }else if(c == '('){
        /* Left paren */
        parser->token_type = OBJ_TOKEN_TYPE_LPAREN;
        OBJ_PARSER_GETC()
    }else if(c == ')'){
        /* Right paren */
        parser->token_type = OBJ_TOKEN_TYPE_RPAREN;
        OBJ_PARSER_GETC()
    }else if(c == ':'){
        /* Colon */
        parser->token_type = OBJ_TOKEN_TYPE_COLON;
        OBJ_PARSER_GETC()
    }else if(c == ' '){
        /* Whitespace */
        parser->token_type = OBJ_TOKEN_TYPE_WHITESPACE;
        do{
            OBJ_PARSER_GETC()
        }while(c == ' ');
    }else if(c == '#' || c == ';'){
        /* Comment or linestring */
        parser->token_type = c == '#'?
            OBJ_TOKEN_TYPE_COMMENT:
            OBJ_TOKEN_TYPE_LINESTRING;
        do{
            OBJ_PARSER_GETC()
        }while(c != '\n' && c != EOF);
    }else if(c == '"'){
        /* String */
        parser->token_type = OBJ_TOKEN_TYPE_STRING;
        do{
            OBJ_PARSER_GETC()
            if(c == '\\'){
                OBJ_PARSER_GETC()
            }
        }while(c != '"' && c != EOF);
        if(c == '"')OBJ_PARSER_GETC()
    }else if(c == '['){
        /* Long Symbol */
        parser->token_type = OBJ_TOKEN_TYPE_LONGSYM;
        do{
            OBJ_PARSER_GETC()
            if(c == '\\'){
                OBJ_PARSER_GETC()
            }
        }while(c != ']' && c != EOF);
        if(c == ']')OBJ_PARSER_GETC()
    }else if(c == '{'){
        /* Typecast */
        parser->token_type = OBJ_TOKEN_TYPE_TYPECAST;
        do{
            OBJ_PARSER_GETC()
            if(c == '\\'){
                OBJ_PARSER_GETC()
            }
        }while(c != '}' && c != EOF);
        if(c == '}')OBJ_PARSER_GETC()
    }else if((c >= '0' && c <= '9') || strchr(ASCII_OPERATORS, c)){
        if(c == '-'){
            /* Integers and operators can both start with '-' */
            OBJ_PARSER_GETC()
        }
        if(c >= '0' && c <= '9'){
            /* Integer */
            parser->token_type = OBJ_TOKEN_TYPE_INT;
            do{
                OBJ_PARSER_GETC()
            }while(c >= '0' && c <= '9');
        }else if(strchr(ASCII_OPERATORS, c)){
            /* Operator */
            parser->token_type = OBJ_TOKEN_TYPE_OPER;
            do{
                OBJ_PARSER_GETC()
            }while(strchr(ASCII_OPERATORS, c));
        }else{
            /* We're parsing the token "-" by itself! */
            parser->token_type = OBJ_TOKEN_TYPE_OPER;
        }
    }else if(c == '_' || strchr(ASCII_LOWER, c) || strchr(ASCII_UPPER, c)){
        /* Name */
        parser->token_type = OBJ_TOKEN_TYPE_NAME;
        do{
            OBJ_PARSER_GETC()
        }while(
            c == '_' ||
            (c >= '0' && c <= '9') ||
            strchr(ASCII_LOWER, c) ||
            strchr(ASCII_UPPER, c)
        );
    }else{
        /* Invalid character: treat it like EOF, return empty token */
        parser->token_type = OBJ_TOKEN_TYPE_INVALID;
        obj_parser_errmsg(parser, __func__);
        fprintf(stderr, "Invalid character: %c (#%i)\n",
            isprint(c)? (char)c: ' ', c);
        return 1;
    }

    parser->token_len = &parser->data[parser->pos] - parser->token;

    if(
        !parser->line_col_is_set &&
        parser->token_type != OBJ_TOKEN_TYPE_WHITESPACE &&
        parser->token_type != OBJ_TOKEN_TYPE_NEWLINE
    ){
        parser->line_col = parser->token_col;
        parser->line_col_is_set = true;
    }

#   ifdef COBJ_DEBUG_TOKENS
    fprintf(stderr, "TOKEN: \"%.*s\"\n",
        size_to_int(parser->token_len, 256), parser->token);
#   endif

    return 0;

#   undef OBJ_PARSER_GETC
}

char *obj_parser_get_unescaped_token(
    obj_parser_t *parser, const char *token, size_t token_len,
    size_t *unescaped_token_len_ptr
){
    char *unescaped_token = obj_parser_get_token_buffer(
        parser, parser->token_len);
    if(!unescaped_token)return NULL;
    size_t unescaped_token_len = 0;
    for(size_t i = 0; i < token_len; i++){
        char c = token[i];
        if(c == '\\'){
            if(i >= token_len - 1){
                fprintf(stderr, "%s: '\\' at end of token: \"%.*s\"\n",
                    __func__, size_to_int(token_len, -1), token);
                return NULL;
            }
            c = token[++i];
            if(c == 'n')c = '\n';
        }
        unescaped_token[unescaped_token_len++] = c;
    }
    *unescaped_token_len_ptr = unescaped_token_len;
    return unescaped_token;
}

obj_string_t *obj_parser_get_string(obj_parser_t *parser){
    if(!obj_parser_token_is_string(parser)){
        obj_parser_errmsg(parser, __func__);
        fprintf(stderr, "Expected string\n");
        return NULL;
    }
    if(parser->token_type == OBJ_TOKEN_TYPE_LINESTRING){
        return obj_pool_string_add_raw(
            parser->pool, parser->token + 1, parser->token_len - 1);
    }
    size_t token_len;
    char *token = obj_parser_get_unescaped_token(parser,
        parser->token + 1, parser->token_len - 2, &token_len);
    if(!token)return NULL;
    return obj_pool_string_add_raw(
        parser->pool, token, token_len);
}

obj_sym_t *obj_parser_get_sym(obj_parser_t *parser){
    if(!obj_parser_token_is_sym(parser)){
        obj_parser_errmsg(parser, __func__);
        fprintf(stderr, "Expected sym\n");
        return NULL;
    }
    if(parser->token_type != OBJ_TOKEN_TYPE_LONGSYM){
        return obj_symtable_get_sym_raw(
            parser->pool->symtable, parser->token,
            parser->token_len);
    }
    size_t token_len;
    char *token = obj_parser_get_unescaped_token(parser,
        parser->token + 1, parser->token_len - 2, &token_len);
    if(!token)return NULL;
    return obj_symtable_get_sym_raw(
        parser->pool->symtable, token, token_len);
}

obj_t *obj_parser_parse(obj_parser_t *parser){
    obj_t *lst = NULL;
    obj_t **tail = &lst;
    int typecast = OBJ_TYPE_UNDEFINED;

    if(parser->use_extended_types){
        fprintf(stderr, "%s: Extended types not supported yet!\n",
            __func__);
        return NULL;
    }

    for(;;){
        if(obj_parser_get_token(parser))return NULL;
        if(parser->token_type == OBJ_TOKEN_TYPE_EOF)break;
        if(parser->line_col_is_set){
            while(
                parser->stack &&
                parser->stack->token_type == OBJ_TOKEN_TYPE_COLON &&
                parser->token_row > parser->stack->token_row &&
                parser->line_col <= parser->stack->line_col
            ){
                if(!(tail = obj_parser_stack_pop(parser, tail)))return NULL;
            }
        }

        obj_t *obj = NULL;
        switch(parser->token_type){
            case OBJ_TOKEN_TYPE_INT: {
                int n = 0;
                for(int i = 0; i < parser->token_len; i++){
                    int digit = parser->token[i] - '0';
                    n *= 10;
                    n += digit;
                }
                obj = obj_pool_add_int(parser->pool, n);
                if(!obj)return NULL;
                break;
            }
            case OBJ_TOKEN_TYPE_NAME:
            case OBJ_TOKEN_TYPE_OPER:
            case OBJ_TOKEN_TYPE_LONGSYM: {
                obj_sym_t *sym = obj_parser_get_sym(parser);
                if(!sym)return NULL;
                obj = obj_pool_add_sym(parser->pool, sym);
                if(!obj)return NULL;
                break;
            }
            case OBJ_TOKEN_TYPE_STRING:
            case OBJ_TOKEN_TYPE_LINESTRING: {
                obj_string_t *string = obj_parser_get_string(parser);
                if(!string)return NULL;
                obj = obj_pool_add_str(parser->pool, string);
                if(!obj)return NULL;
                break;
            }
            case OBJ_TOKEN_TYPE_COLON:
            case OBJ_TOKEN_TYPE_LPAREN: {
                *tail = obj_pool_add_cell(parser->pool, NULL, NULL);
                if(!*tail)return NULL;
                obj_t **next_tail = &OBJ_TAIL(*tail);

                if(!obj_parser_stack_push(parser, next_tail))return NULL;
                tail = &OBJ_HEAD(*tail);
                break;
            }
            case OBJ_TOKEN_TYPE_RPAREN: {
                while(
                    parser->stack &&
                    parser->stack->token_type != OBJ_TOKEN_TYPE_LPAREN
                ){
                    if(!(tail = obj_parser_stack_pop(parser, tail))){
                        return NULL;
                    }
                }
                if(!parser->stack){
                    obj_parser_errmsg(parser, __func__);
                    fprintf(stderr, "Too many closing parentheses\n");
                    return NULL;
                }
                if(!(tail = obj_parser_stack_pop(parser, tail)))return NULL;
                break;
            }
            case OBJ_TOKEN_TYPE_TYPECAST: {
                if(!parser->use_extended_types){
                    /* It's cool, just carry on like you never saw a
                    typecast token */
                }else if(obj_parser_token_eq(parser, "{array}")){
                    typecast = OBJ_TYPE_ARRAY;
                }else if(obj_parser_token_eq(parser, "{dict}")){
                    typecast = OBJ_TYPE_DICT;
                }else if(obj_parser_token_eq(parser, "{struct}")){
                    typecast = OBJ_TYPE_STRUCT;
                }else{
                    obj_parser_errmsg(parser, __func__);
                    fprintf(stderr, "Unrecognized typecast\n");
                    return NULL;
                }
                break;
            }
            default: break;
        }

        if(obj){
            *tail = obj_pool_add_cell(parser->pool, obj, NULL);
            if(!*tail)return NULL;
            tail = &OBJ_TAIL(*tail);
        }

        if(parser->token_type != OBJ_TOKEN_TYPE_TYPECAST){
            typecast = OBJ_TYPE_NIL;
        }
    }

    while(
        parser->stack &&
        parser->stack->token_type == OBJ_TOKEN_TYPE_COLON
    ){
        if(!(tail = obj_parser_stack_pop(parser, tail)))return NULL;
    }

    if(parser->stack){
        obj_parser_errmsg(parser, __func__);
        fprintf(stderr, "Too many opening parentheses\n");
        while(parser->stack){
            fprintf(stderr, "  [row=%zu col=%zu type=%s]\n",
                parser->stack->token_row+1,
                parser->stack->line_col+1,
                obj_token_type_msg(parser->stack->token_type));
            parser->stack = parser->stack->next;
        }
        return NULL;
    }

    *tail = obj_pool_add_nil(parser->pool);
    if(!*tail)return NULL;
    return lst;
}

obj_t *obj_parse(obj_pool_t *pool, const char *filename,
    const char *data, size_t data_len
){
    obj_parser_t _parser, *parser = &_parser;
    obj_parser_init(parser, pool, filename, data, data_len);
    obj_t *obj = obj_parser_parse(parser);
    obj_parser_cleanup(parser);
    return obj;
}



/******
* obj *
******/

static void obj_fprint(obj_t *obj, FILE *file, int depth){
    int type = OBJ_TYPE(obj);
    while(type == OBJ_TYPE_BOX){
        obj = OBJ_CONTENTS(obj);
        type = OBJ_TYPE(obj);
    }
    switch(type){
        case OBJ_TYPE_BOOL:
            fprintf(file, "{bool}%c", OBJ_BOOL(obj)? 'T': 'F');
            break;
        case OBJ_TYPE_INT:
            fprintf(file, "%i", OBJ_INT(obj));
            break;
        case OBJ_TYPE_STR: {
            obj_string_t *s = OBJ_STRING(obj);
            obj_string_fprint(s, file);
            break;
        }
        case OBJ_TYPE_SYM: {
            obj_sym_t *y = OBJ_SYM(obj);
            obj_sym_fprint(y, file);
            break;
        }
        case OBJ_TYPE_CELL:
        case OBJ_TYPE_TAIL:
        case OBJ_TYPE_NIL: {
            if(type == OBJ_TYPE_TAIL)fprintf(file, "{tail}");
            fprintf(file, ":");
            while(OBJ_TYPE(obj) != OBJ_TYPE_NIL){
                putc('\n', file);
                _print_tabs(file, depth+2);
                obj_fprint(OBJ_HEAD(obj), file, depth+2);
                obj = OBJ_TAIL(obj);
            }
            break;
        }
        case OBJ_TYPE_ARRAY: {
            fprintf(file, "{array}:");
            int len = OBJ_ARRAY_LEN(obj);
            for(int i = 0; i < len; i++){
                putc('\n', file);
                _print_tabs(file, depth+2);
                obj_fprint(OBJ_ARRAY_IGET(obj, i), file, depth+2);
            }
            break;
        }
        case OBJ_TYPE_DICT: {
            fprintf(file, "{dict}:");
            obj_dict_t *dict = OBJ_DICT(obj);
            obj_dict_fprint(dict, file, depth + 2);
            break;
        }
        case OBJ_TYPE_STRUCT: {
            fprintf(file, "{struct}:");
            int n_keys = OBJ_STRUCT_KEYS_LEN(obj);
            for(int i = 0; i < n_keys; i++){
                obj_sym_t *key = OBJ_SYM(OBJ_STRUCT_IGET_KEY(obj, i));
                obj_t *val = OBJ_STRUCT_IGET_VAL(obj, i);

                putc('\n', file);
                _print_tabs(file, depth+2);

                obj_sym_fprint(key, file);

                putc(' ', file);
                obj_fprint((obj_t*)val, file, depth+2);
            }
            break;
        }
        case OBJ_TYPE_NULL:
            fprintf(file, "{null}null");
            break;
        default:
            fprintf(file, "{unknown}unknown");
            break;
    }
}

void obj_dump(obj_t *obj, FILE *file, int depth){
    _print_tabs(file, depth);
    obj_fprint(obj, file, depth);
    putc('\n', file);
}

obj_t *obj_resolve(obj_t *obj){
    while(OBJ_TYPE(obj) == OBJ_TYPE_BOX)obj = OBJ_CONTENTS(obj);
    return obj;
}

obj_t *obj_list_get(obj_t *obj, obj_sym_t *sym){
    while(obj && OBJ_TYPE(obj) == OBJ_TYPE_CELL){
        obj_t *head = OBJ_HEAD(obj);
        obj_t *tail = OBJ_TAIL(obj);
        if(!tail || OBJ_TYPE(tail) != OBJ_TYPE_CELL)return NULL;
        if(head && OBJ_TYPE(head) == OBJ_TYPE_SYM && OBJ_SYM(head) == sym){
            return OBJ_HEAD(tail);
        }
        obj = OBJ_TAIL(tail);
    }
    return NULL;
}

obj_t *obj_list_iget(obj_t *obj, int i){
    while(obj && OBJ_TYPE(obj) == OBJ_TYPE_CELL){
        if(i <= 0)return OBJ_HEAD(obj);
        obj = OBJ_TAIL(obj);
        i--;
    }
    return NULL;
}

int obj_list_len(obj_t *obj){
    int len = 0;
    while(obj && OBJ_TYPE(obj) == OBJ_TYPE_CELL){
        obj = OBJ_TAIL(obj);
        len++;
    }
    return len;
}

obj_t *obj_struct_get(obj_t *obj, obj_sym_t *sym){
    int n_keys = OBJ_STRUCT_KEYS_LEN(obj);
    for(int i = 0; i < n_keys; i++){
        obj_sym_t *key = OBJ_SYM(OBJ_STRUCT_IGET_KEY(obj, i));
        if(key != sym)continue;
        return OBJ_STRUCT_IGET_VAL(obj, i);
    }
    return NULL;
}

int obj_len(obj_t *obj){
    if(!obj)return 0;
    int type = OBJ_TYPE(obj);
    if(type == OBJ_TYPE_NIL){
        return 0;
    }else if(type == OBJ_TYPE_CELL){
        return obj_list_len(obj);
    }else if(type == OBJ_TYPE_ARRAY){
        return OBJ_ARRAY_LEN(obj);
    }else if(type == OBJ_TYPE_DICT){
        return OBJ_DICT_KEYS_LEN(obj);
    }else if(type == OBJ_TYPE_STRUCT){
        return OBJ_STRUCT_KEYS_LEN(obj);
    }else{
        return 0;
    }
}

obj_t *obj_get(obj_t *obj, obj_sym_t *sym){
    if(!obj)return NULL;
    int type = OBJ_TYPE(obj);
    if(type == OBJ_TYPE_NIL){
        return NULL;
    }else if(type == OBJ_TYPE_CELL){
        return obj_list_get(obj, sym);
    }else if(type == OBJ_TYPE_DICT){
        return obj_dict_get(OBJ_DICT(obj), sym);
    }else if(type == OBJ_TYPE_STRUCT){
        return obj_struct_get(obj, sym);
    }else{
        return NULL;
    }
}

obj_t *obj_iget(obj_t *obj, int i){
    if(!obj)return NULL;
    int type = OBJ_TYPE(obj);
    if(type == OBJ_TYPE_NIL){
        return NULL;
    }else if(type == OBJ_TYPE_CELL){
        return obj_list_iget(obj, i);
    }else if(type == OBJ_TYPE_ARRAY){
        return OBJ_ARRAY_IGET(obj, i);
    }else{
        return NULL;
    }
}


#endif