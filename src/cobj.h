#ifndef _COBJ_H_
#define _COBJ_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct obj obj_t;
typedef struct obj_string obj_string_t;
typedef struct obj_symtable obj_symtable_t;
typedef struct obj_pool obj_pool_t;
typedef struct obj_parser obj_parser_t;

enum {
    OBJ_TYPE_INT,
    OBJ_TYPE_SYM,
    OBJ_TYPE_STR,
    OBJ_TYPE_NIL,
    OBJ_TYPE_LST,
};

struct obj {
    int type;
    union {
        int i;
        obj_string_t *s;
        obj_t *next;
    } u;
};

struct obj_string {
    size_t len;
    char *data;
};

struct obj_symtable {
    obj_string_t *syms;
    size_t syms_len;
};

struct obj_pool {
    obj_symtable_t *symtable;

    obj_t *objs;
    size_t objs_len;

    obj_string_t *strings;
    size_t strings_len;
};

struct obj_parser {
    obj_pool_t *pool;
    const char *data;
    size_t data_len;
    size_t pos;
    size_t row;
    size_t col;
    const char *token;
    size_t token_len;
};



/***************
* obj_symtable *
***************/

void obj_symtable_init(obj_symtable_t *table){
    memset(table, 0, sizeof(*table));
}

void obj_symtable_cleanup(obj_symtable_t *table){
    free(table->syms);
}


/***********
* obj_pool *
***********/

void obj_pool_init(obj_pool_t *pool, obj_symtable_t *symtable){
    memset(pool, 0, sizeof(*pool));
    pool->symtable = symtable;
}

void obj_pool_cleanup(obj_pool_t *pool){
    free(pool->objs);
    for(size_t i = 0; i < pool->strings_len; i++){
        free(pool->strings[i].data);
    }
    free(pool->strings);
}

obj_t *obj_pool_objs_alloc(obj_pool_t *pool, size_t n_objs){

    /* realloc pool->objs */
    size_t old_len = pool->objs_len;
    size_t len = old_len + n_objs;
    obj_t *objs = realloc(pool->objs, sizeof(*objs) * len);
    if(!objs){
        fprintf(stderr,
            "%s: Couldn't reallocate number of objs from %zu to %zu\n",
            __func__, pool->objs_len, len);
        perror("realloc");
        return NULL;
    }
    pool->objs = objs;
    pool->objs_len = len;

    return &pool->objs[old_len];
}

obj_t *obj_pool_add_int(obj_pool_t *pool, int i){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->type = OBJ_TYPE_INT;
    obj->u.i = i;
    return obj;
}

obj_t *obj_pool_add_sym(obj_pool_t *pool, obj_string_t *string){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->type = OBJ_TYPE_SYM;
    obj->u.s = string;
    return obj;
}

obj_t *obj_pool_add_str(obj_pool_t *pool, obj_string_t *string){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->type = OBJ_TYPE_STR;
    obj->u.s = string;
    return obj;
}

obj_t *obj_pool_add_nil(obj_pool_t *pool){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->type = OBJ_TYPE_NIL;
    return obj;
}

obj_t *obj_pool_add_lst(obj_pool_t *pool){
    /* NOTE: lst objects are added in an invalid state: there is
    no obj immediately after them, and their tail is NULL (instead
    of pointing to a nil object coming after them).
    So one is expected to add a list, then add more objs after it,
    eventually capping the lst by pointing it at a nil. */
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->type = OBJ_TYPE_LST;
    return obj;
}

obj_string_t *obj_pool_strings_alloc(obj_pool_t *pool, size_t len){

    /* realloc pool->strings */
    size_t strings_len = pool->strings_len + 1;
    obj_string_t *strings = realloc(pool->strings, sizeof(*strings) * strings_len);
    if(!strings){
        fprintf(stderr,
            "%s: Couldn't reallocate number of strings from %zu to %zu\n",
            __func__, pool->strings_len, strings_len);
        perror("realloc");
        return NULL;
    }
    pool->strings = strings;
    pool->strings_len = strings_len;

    /* alloc new string (new last entry of pool->strings) */
    obj_string_t *string = &strings[strings_len - 1];
    string->data = NULL;
    string->len = 0;
    char *data = malloc(len);
    if(!data){
        fprintf(stderr,
            "%s: Couldn't allocate string of %zu bytes\n",
                __func__, len);
        perror("malloc");
        return NULL;
    }
    string->data = data;
    string->len = len;

    return string;
}

obj_string_t *obj_pool_strings_add_raw(obj_pool_t *pool, const char *data, size_t len){
    /* "raw" meaning length is specified, instead of NUL-terminated */
    obj_string_t *string = obj_pool_strings_alloc(pool, len);
    if(!string)return NULL;
    memcpy(string->data, data, len);
    return string;
}

obj_string_t *obj_pool_strings_add(obj_pool_t *pool, const char *data){
    return obj_pool_strings_add_raw(pool, data, strlen(data));
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
    // nothin
}


void obj_parser_get_token(obj_parser_t *parser){
    /* Updates parser->token, parser->token_len.
    If end of parser->data is reached (that is,
    parser->pos >= parser->data_len), then parser->token is set to NULL.
    NOTE: parser->data is not allowed to contain NUL bytes. */

#   define OBJ_PARSER_GETC() { \
        if(parser->pos >= parser->data_len)goto eof; \
        c = parser->data[parser->pos++]; \
        if(c == '\n'){ \
            parser->row++; \
            parser->col = 0; \
        }else{ \
            parser->col++; \
        } \
    }

    int c;

    /* consume whitespace & comments */
    for(;;){
        OBJ_PARSER_GETC()
        if(c != ' ' && c != '\n')break;
        if(c == '#'){
            for(;;){
                OBJ_PARSER_GETC()
                if(c == '\n')break;
            }
        }
    }

    size_t token_start = parser->pos;

    /* consume token */
    for(;;){
        if(c == ' ' || c == '\n')break;
        OBJ_PARSER_GETC()
    }

    parser->token = parser->data + token_start;
    parser->token_len = parser->pos - token_start;
    return;

eof:
    parser->token = NULL;
    parser->token_len = 0;
    return;

#   undef OBJ_PARSER_GETC
}

obj_t *obj_parser_parse(obj_parser_t *parser){
    return NULL;
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
    switch(obj->type){
        case OBJ_TYPE_INT:
            fprintf(file, "%i", obj->u.i);
            break;
        case OBJ_TYPE_STR:
            putc('"', file);
        case OBJ_TYPE_SYM: {
            obj_string_t *s = obj->u.s;

            /* Are we safe??? */
            int len = (int)s->len;
            if(len < 0)len = 0;

            fprintf(file, "%.*s", len, s->data);
            if(obj->type == OBJ_TYPE_STR)putc('"', file);
            break;
        }
        case OBJ_TYPE_LST:
        case OBJ_TYPE_NIL: {
            fprintf(file, ":");
            obj_t *lst = obj;
            while(lst->type != OBJ_TYPE_NIL){
                putc('\n', file);
                _print_tabs(file, depth);
                _obj_dump(lst+1, file, depth+2);
                lst = lst->u.next;
            }
            break;
        }
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