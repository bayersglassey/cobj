#ifndef _COBJ_H_
#define _COBJ_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* Users are free to use the unmasked bits of obj->tag
however they wish */
#define OBJ_TYPE_MASK 0x7

#define OBJ_TYPE(obj) ((obj)[0].tag & OBJ_TYPE_MASK)
#define OBJ_INT(obj) (obj)[0].u.i
#define OBJ_STRING(obj) (obj)[0].u.s
#define OBJ_HEAD(obj) (obj)[0].u.o
#define OBJ_TAIL(obj) (obj)[1].u.o

#ifndef OBJ_POOL_CHUNK_LEN
#   define OBJ_POOL_CHUNK_LEN 1024
#endif

const char ASCII_OPERATORS[] = "!$%&'*+,-./<=>?@[]^`{|}~";
const char ASCII_LOWER[] = "abcdefghijklmnopqrstuvwxyz";
const char ASCII_UPPER[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct obj obj_t;
typedef struct obj_string obj_string_t;
typedef struct obj_string_list obj_string_list_t;
typedef struct obj_symtable obj_symtable_t;
typedef struct obj_pool obj_pool_t;
typedef struct obj_pool_chunk obj_pool_chunk_t;
typedef struct obj_parser obj_parser_t;

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
    OBJ_TOKEN_TYPE_SPACE,
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

struct obj_symtable {
    obj_string_t *syms;
    size_t syms_len;
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
    for(obj_pool_chunk_t *chunk = pool->chunk_list; chunk;){
        obj_pool_chunk_t *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    for(obj_string_list_t *string_list = pool->string_list; string_list;){
        obj_string_list_t *next = string_list->next;
        free(string_list->string.data);
        free(string_list);
        string_list = next;
    }
}

void obj_pool_dump(obj_pool_t *pool, FILE *file){
    fprintf(file, "OBJ POOL %p:\n", pool);
    fprintf(file, "  CHUNKS:\n");
    for(obj_pool_chunk_t *chunk = pool->chunk_list; chunk;){
        fprintf(file, "    CHUNK %p: %zu/%zu\n",
            chunk, chunk->len, (size_t)OBJ_POOL_CHUNK_LEN);
        chunk = chunk->next;
    }
    fprintf(file, "  STRINGS:\n");
    for(obj_string_list_t *string_list = pool->string_list; string_list;){
        obj_string_t *string = &string_list->string;
        int len = (int)string->len;
        if(len < 0)len = 0;
        if(len > 40)len = 40;
        fprintf(file, "    STRING %p: %.*s\n", string, len, string->data);
        string_list = string_list->next;
    }
}

obj_t *obj_pool_objs_alloc(obj_pool_t *pool, size_t n_objs){
    obj_pool_chunk_t *chunk = pool->chunk_list;
    if(!chunk || chunk->len + n_objs >= OBJ_POOL_CHUNK_LEN){
        obj_pool_chunk_t *new_chunk = malloc(sizeof(*new_chunk));
        if(new_chunk == NULL){
            fprintf(stderr, "%s: Couldn't allocate new chunk\n", __func__);
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

obj_t *obj_pool_add_sym(obj_pool_t *pool, obj_string_t *string){
    obj_t *obj = obj_pool_objs_alloc(pool, 1);
    if(!obj)return NULL;
    obj->tag = OBJ_TYPE_SYM;
    OBJ_STRING(obj) = string;
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
        fprintf(stderr, "%s: Couldn't allocate new string list node\n",
            __func__);
        perror("malloc");
        return NULL;
    }
    string_list->next = pool->string_list;
    pool->string_list = string_list;

    /* alloc new string (new last entry of pool->strings) */
    obj_string_t *string = &string_list->string;
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
    // nothin
}


void obj_parser_get_token(obj_parser_t *parser){
    /* Updates parser->token, parser->token_len.
    If end of parser->data is reached (that is,
    parser->pos >= parser->data_len), then parser->token is set to NULL.
    NOTE: parser->data is not allowed to contain NUL bytes. */

#   define OBJ_PARSER_GETC() \
        if(parser->pos >= parser->data_len - 1){ \
            c = EOF; \
        }else { \
            parser->pos++; \
            if(c == '\n'){ \
                parser->row++; \
                parser->col = 0; \
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
        parser->token_type = OBJ_TOKEN_TYPE_SPACE;
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
        fprintf(stderr,
            "%s [pos=%zu row=%zu col=%zu]: Invalid character: %c (#%i)\n",
            __func__, parser->pos, parser->row, parser->col,
            isprint(c)? (char)c: ' ', c);
    }

    parser->token_len = &parser->data[parser->pos] - parser->token;
    return;

#   undef OBJ_PARSER_GETC
}

obj_t *obj_parser_parse(obj_parser_t *parser){
    obj_t *lst = NULL;
    obj_t **tail = &lst;
    for(;;){
        obj_parser_get_token(parser);
        if(!parser->token_len)break;
        /* fprintf(stderr, "TOKEN: [%.*s]\n",
            (int)parser->token_len, parser->token); */

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
            }
            case OBJ_TOKEN_TYPE_NAME:
            case OBJ_TOKEN_TYPE_OPER: {
                obj_string_t *string = obj_pool_string_add_raw(
                    parser->pool, parser->token, parser->token_len);
                if(!string)return NULL;
                obj = obj_pool_add_sym(parser->pool, string);
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
                break;
            }
            case OBJ_TOKEN_TYPE_RPAREN: {
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
        case OBJ_TYPE_STR:
            putc('"', file);
        case OBJ_TYPE_SYM: {
            obj_string_t *s = OBJ_STRING(obj);

            /* Are we safe??? */
            int len = (int)s->len;
            if(len < 0)len = 0;

            fprintf(file, "%.*s", len, s->data);
            if(type == OBJ_TYPE_STR)putc('"', file);
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