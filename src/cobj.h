#ifndef _COBJ_H_
#define _COBJ_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* #define COBJ_DEBUG_TOKENS */


/* Users are free to use the unmasked bits of obj->tag
however they wish */
#define OBJ_TYPE_MASK 0x7

#define OBJ_TYPE(obj) ((obj)[0].tag & OBJ_TYPE_MASK)
#define OBJ_INT(obj) (obj)[0].u.i
#define OBJ_SYM(obj) (obj)[0].u.y
#define OBJ_STRING(obj) (obj)[0].u.s
#define OBJ_HEAD(obj) (obj)[0].u.o
#define OBJ_TAIL(obj) (obj)[1].u.o

#ifndef OBJ_POOL_CHUNK_LEN
#   define OBJ_POOL_CHUNK_LEN 1024
#endif

#define OBJ_SYMTABLE_DEFAULT_SIZE 16

const char ASCII_OPERATORS[] = "!$%&'*+,-./<=>?@[]^`{|}~";
const char ASCII_LOWER[] = "abcdefghijklmnopqrstuvwxyz";
const char ASCII_UPPER[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct obj obj_t;
typedef struct obj_string obj_string_t;
typedef struct obj_string_list obj_string_list_t;
typedef struct obj_sym obj_sym_t;
typedef struct obj_symtable obj_symtable_t;
typedef struct obj_pool obj_pool_t;
typedef struct obj_pool_chunk obj_pool_chunk_t;
typedef struct obj_parser obj_parser_t;
typedef struct obj_parser_stack obj_parser_stack_t;

enum {
    OBJ_TYPE_INT,
    OBJ_TYPE_SYM,
    OBJ_TYPE_STR,
    OBJ_TYPE_NIL,
    OBJ_TYPE_CELL_HEAD,
    OBJ_TYPE_CELL_TAIL,
};

enum {
    OBJ_TOKEN_TYPE_INVALID,
    OBJ_TOKEN_TYPE_EOF,
    OBJ_TOKEN_TYPE_WHITESPACE,
    OBJ_TOKEN_TYPE_COMMENT,
    OBJ_TOKEN_TYPE_NEWLINE,
    OBJ_TOKEN_TYPE_INT,
    OBJ_TOKEN_TYPE_NAME,
    OBJ_TOKEN_TYPE_OPER,
    OBJ_TOKEN_TYPE_STRING,
    OBJ_TOKEN_TYPE_LINESTRING,
    OBJ_TOKEN_TYPE_LPAREN,
    OBJ_TOKEN_TYPE_RPAREN,
    OBJ_TOKEN_TYPE_COLON,
};

struct obj {
    int tag;
    union {
        int i;
        obj_sym_t *y;
        obj_string_t *s;
        obj_t *o;
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

struct obj_pool {
    obj_symtable_t *symtable;

    obj_pool_chunk_t *chunk_list;

    obj_string_list_t *string_list;
};

struct obj_pool_chunk {
    obj_pool_chunk_t *next;
    obj_t objs[OBJ_POOL_CHUNK_LEN];
    size_t len;
};

struct obj_parser {
    obj_pool_t *pool;
    const char *data;
    size_t data_len;
    size_t pos;
    size_t row;
    size_t col;
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



/************
* utilities *
************/

int obj_hash(const char *s, int len){
    /* The classic hash by Bernstein */
    int hash = 5381;
    for(int i = 0; i < len; i++){
        hash = hash * 33 + s[i];
    }
    return hash;
}

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
        int len = sym->string.len >= 256? 256: sym->string.len;
        fprintf(file, ": %.*s\n", len, sym->string.data);
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
    obj_sym_t **syms = calloc(sizeof(*syms) * syms_len, 1);
    if(!syms){
        obj_symtable_errmsg(table, __func__);
        fprintf(stderr,
            "Trying to allocate new syms of size %zu. ", syms_len);
        perror("malloc");
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

    obj_sym_t *sym = malloc(sizeof(*sym));
    if(!sym){
        obj_symtable_errmsg(table, __func__);
        fprintf(stderr, "While getting sym for %.*s: ",
            text_len >= 256? 256: (int)text_len, text);
        fprintf(stderr, "Couldn't allocate sym. ");
        perror("malloc");
        return NULL;
    }
    obj_string_t *string = &sym->string;
    if(!obj_string_init(string, text_len)){
        obj_symtable_errmsg(table, __func__);
        fprintf(stderr, "While getting sym for %.*s: ",
            text_len >= 256? 256: (int)text_len, text);
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
            if(
                sym->string.len == text_len &&
                !strncmp(sym->string.data, text, text_len)
            ){
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
* obj_pool *
***********/

void obj_pool_init(obj_pool_t *pool, obj_symtable_t *symtable){
    memset(pool, 0, sizeof(*pool));
    pool->symtable = symtable;
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
        int len = (int)string->len;
        if(len < 0)len = 0;
        if(len > 40)len = 40;
        fprintf(file, "    STRING %p (%zu): %.*s\n",
            string, string->len, len, string->data);
    }
}

obj_t *obj_pool_objs_alloc(obj_pool_t *pool, size_t n_objs){
    obj_pool_chunk_t *chunk = pool->chunk_list;
    if(!chunk || chunk->len + n_objs >= OBJ_POOL_CHUNK_LEN){
        obj_pool_chunk_t *new_chunk = malloc(sizeof(*new_chunk));
        if(new_chunk == NULL){
            obj_pool_errmsg(pool, __func__);
            perror("malloc");
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
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_NIL;
    return obj;
}

obj_t *obj_pool_add_cell(obj_pool_t *pool, obj_t *head, obj_t *tail){
    obj_t *obj = obj_pool_objs_alloc(pool, 2);
    if(!obj)return NULL;
    obj[0].tag = OBJ_TYPE_CELL_HEAD;
    obj[1].tag = OBJ_TYPE_CELL_TAIL;
    OBJ_HEAD(obj) = head;
    OBJ_TAIL(obj) = tail;
    return obj;
}

obj_string_t *obj_pool_string_alloc(obj_pool_t *pool, size_t len){

    /* add new linked list entry */
    obj_string_list_t *string_list = malloc(sizeof(*string_list));
    if(!string_list){
        fprintf(stderr, "%s: Couldn't allocate new string list node. ",
            __func__);
        perror("malloc");
        return NULL;
    }
    string_list->next = pool->string_list;
    pool->string_list = string_list;

    /* alloc new string (new last entry of pool->strings) */
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


/*************
* obj_parser *
*************/

void obj_parser_init(
    obj_parser_t *parser, obj_pool_t *pool,
    const char *data, size_t data_len
){
    memset(parser, 0, sizeof(*parser));
    parser->pool = pool;
    parser->data = data;
    parser->data_len = data_len;
}

void obj_parser_cleanup(obj_parser_t *parser){
    while(parser->stack){
        obj_parser_stack_t *next = parser->stack->next;
        free(parser->stack);
        parser->stack = next;
    }
    while(parser->free_stack){
        obj_parser_stack_t *next = parser->free_stack->next;
        free(parser->free_stack);
        parser->free_stack = next;
    }
}

void obj_parser_dump(obj_parser_t *parser, FILE *file){
    fprintf(file, "OBJ PARSER %p:\n", parser);
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
}

obj_parser_stack_t *obj_parser_stack_push(obj_parser_t *parser, obj_t **tail){
    obj_parser_stack_t *stack;
    if(parser->free_stack){
        stack = parser->free_stack;
        parser->free_stack = parser->free_stack->next;
    }else{
        stack = malloc(sizeof(*stack));
        if(!stack){
            obj_parser_errmsg(parser, __func__);
            perror("malloc");
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
        }else{
            /* Operator */
            parser->token_type = OBJ_TOKEN_TYPE_OPER;
            do{
                OBJ_PARSER_GETC()
            }while(strchr(ASCII_OPERATORS, c));
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

    return 0;

#   undef OBJ_PARSER_GETC
}

obj_t *obj_parser_parse(obj_parser_t *parser){
    obj_t *lst = NULL;
    obj_t **tail = &lst;
    for(;;){
        if(obj_parser_get_token(parser))return NULL;
        if(parser->token_type == OBJ_TOKEN_TYPE_EOF)break;
#       ifdef COBJ_DEBUG_TOKENS
        fprintf(stderr, "TOKEN: [%.*s]\n",
            (int)parser->token_len, parser->token);
#       endif

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
            case OBJ_TOKEN_TYPE_OPER: {
                obj_sym_t *sym = obj_symtable_get_sym_raw(
                    parser->pool->symtable, parser->token, parser->token_len);
                if(!sym)return NULL;
                obj = obj_pool_add_sym(parser->pool, sym);
                if(!obj)return NULL;
                break;
            }
            case OBJ_TOKEN_TYPE_STRING:
            case OBJ_TOKEN_TYPE_LINESTRING: {
                size_t string_len =
                    parser->token_type == OBJ_TOKEN_TYPE_LINESTRING?
                        parser->token_len - 1:
                        parser->token_len - 2;
                obj_string_t *string = obj_pool_string_add_raw(
                    parser->pool, parser->token + 1, string_len);
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
                    if(!(tail = obj_parser_stack_pop(parser, tail)))return NULL;
                }
                if(!parser->stack){
                    obj_parser_errmsg(parser, __func__);
                    fprintf(stderr, "Too many closing parentheses\n");
                    return NULL;
                }
                if(!(tail = obj_parser_stack_pop(parser, tail)))return NULL;
                break;
            }
            default: break;
        }

        if(obj){
            *tail = obj_pool_add_cell(parser->pool, obj, NULL);
            if(!*tail)return NULL;
            tail = &OBJ_TAIL(*tail);
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
        return NULL;
    }

    *tail = obj_pool_add_nil(parser->pool);
    if(!*tail)return NULL;
    return lst;
}

obj_t *obj_parse(obj_pool_t *pool, const char *data, size_t data_len){
    obj_parser_t _parser, *parser = &_parser;
    obj_parser_init(parser, pool, data, data_len);
    obj_t *obj = obj_parser_parse(parser);
    obj_parser_cleanup(parser);
    return obj;
}



/******
* obj *
******/

static void _print_tabs(FILE *file, int depth){
    for(int i = 0; i < depth; i++)putc(' ', file);
}

static void _obj_dump(obj_t *obj, FILE *file, int depth){
    int type = OBJ_TYPE(obj);
    switch(type){
        case OBJ_TYPE_INT:
            fprintf(file, "%i", OBJ_INT(obj));
            break;
        case OBJ_TYPE_STR: {
            putc('"', file);
            obj_string_t *s = OBJ_STRING(obj);

            /* Are we safe??? */
            int len = (int)s->len;
            if(len < 0)len = 0;

            fprintf(file, "%.*s", len, s->data);
            putc('"', file);
            break;
        }
        case OBJ_TYPE_SYM: {
            obj_sym_t *y = OBJ_SYM(obj);
            obj_string_t *s = &y->string;

            /* Are we safe??? */
            int len = (int)s->len;
            if(len < 0)len = 0;

            fprintf(file, "%.*s", len, s->data);
            break;
        }
        case OBJ_TYPE_CELL_HEAD:
        case OBJ_TYPE_NIL: {
            fprintf(file, ":");
            while(OBJ_TYPE(obj) != OBJ_TYPE_NIL){
                putc('\n', file);
                _print_tabs(file, depth+2);
                _obj_dump(OBJ_HEAD(obj), file, depth+2);
                obj = OBJ_TAIL(obj);
            }
            break;
        }
        case OBJ_TYPE_CELL_TAIL:
            fprintf(file, "<tail>");
            break;
        default:
            fprintf(file, "<unknown>");
            break;
    }
}

void obj_dump(obj_t *obj, FILE *file, int depth){
    _print_tabs(file, depth);
    _obj_dump(obj, file, depth);
    putc('\n', file);
}


#endif