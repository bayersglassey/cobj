#ifndef _COBJ_H_
#define _COBJ_H_


typedef struct obj obj_t;
typedef struct obj_string obj_string_t;
typedef struct obj_symtable obj_symtable_t;
typedef struct obj_pool obj_pool_t;

enum {
    OBJ_TYPE_INT,
    OBJ_TYPE_SYM,
    OBJ_TYPE_STR,
    OBJ_TYPE_ARR,
};

struct obj {
    int type;
    union {
        int i;
        char *s;
        char *y;
        size_t len;
    } u;
};

struct obj_string {
    size_t size;
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

int obj_pool_strings_alloc(obj_pool_t *pool, size_t size){

    /* realloc pool-strings */
    size_t len = pool->strings_len + 1;
    obj_string_t *strings = realloc(pool->strings, sizeof(*strings) * len);
    if(!strings){
        fprintf(stderr,
            "%s: Couldn't reallocate number of strings from %zu to %zu\n",
            __func__, pool->strings_len, len);
        perror("realloc");
        return 1;
    }
    pool->strings = strings;

    /* alloc new string (new last entry of pool->strings) */
    obj_string_t *string = &strings[len - 1];
    string->data = NULL;
    string->size = 0;
    char *data = malloc(size);
    if(!data){
        fprintf(stderr,
            "%s: Couldn't allocate string of %zu bytes\n",
                __func__, size);
        perror("malloc");
        return 1;
    }
    string->data = data;
    string->size = size;

    return 0;
}


#endif