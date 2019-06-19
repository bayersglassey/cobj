
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cobj.h"


static char *load_file(const char* filename, size_t *size_ptr){
    const char *ERRMSG = "nothing";
    FILE *file = fopen(filename, "r");
    if(!file){
        ERRMSG = "fopen";
        goto err;
    }
    if(fseek(file, 0, SEEK_END)){
        ERRMSG = "fseek";
        goto err;
    }
    long end = ftell(file);
    if(end < 0){
        ERRMSG = "ftell";
        goto err;
    }
    if(fseek(file, 0, 0)){
        ERRMSG = "fseek";
        goto err;
    }
    size_t size = end;
    char *buffer = malloc(size);
    if(!buffer){
        fprintf(stderr, "Couldn't allocate file buffer (%zu bytes)\n", size);
        ERRMSG = "malloc";
        goto err;
    }
    if(!fread(buffer, size, 1, file)){
        fprintf(stderr, "Couldn't read %zu bytes into file buffer\n", size);
        ERRMSG = "fread";
        goto err;
    }
    if(fclose(file)){
        ERRMSG = "fclose";
        goto err;
    }

    *size_ptr = size;
    return buffer;

err:
    fprintf(stderr, "load_file(%s): ", filename);
    perror(ERRMSG);
    return NULL;
}


static int parse_file(
    const char *filename, const char *data,
    obj_pool_t *pool, obj_t **obj_ptr
){
    return 0;
}


int main(int n_args, char *args[]){

    obj_symtable_t _table, *table=&_table;
    obj_pool_t _pool, *pool=&_pool;
    obj_symtable_init(table);
    obj_pool_init(pool, table);

    for(int i = 1; i < n_args; i++){
        char *arg = args[i];
        if(!strcmp(arg, "-f")){
            if(i >= n_args - 1){
                fprintf(stderr, "Missing arg after %s\n", arg);
                return 1;
            }
            arg = args[++i];

            fprintf(stderr, "Loading file: %s\n", arg);
            size_t size;
            char *buffer = load_file(arg, &size);
            if(!buffer)return 1;
            fprintf(stderr, "Loaded file: %s\n", arg);

            fprintf(stderr, "Parsing file: %s\n", arg);
            obj_t *obj;
            if(parse_file(arg, buffer, pool, &obj))return 1;
            fprintf(stderr, "Parsed file: %s\n", arg);
            free(buffer);
        }else{
            fprintf(stderr, "Unrecognized option: %s\n", arg);
            return 1;
        }
    }

    obj_symtable_cleanup(table);
    obj_pool_cleanup(pool);

    fprintf(stderr, "OK!\n");
    return 0;
}
