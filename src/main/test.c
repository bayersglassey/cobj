
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../cobj.h"


static void print_help(){
    fprintf(stderr,
        "Arguments:\n"
        "  -f FILE    Loads & parses given file\n"
        "  -c TEXT    Parses given text\n"
    );
}

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


static int run_obj_test(obj_pool_t *pool){

    /* Add a string to the pool */
    obj_string_t *string = obj_pool_string_add(pool, "HALLO WARLD!");
    if(!string){
        fprintf(stderr, "%s: Couldn't allocate string\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated string: %.*s\n",
        __func__, (int)string->len, string->data);

    /* Now we'll add some objects to the pool */
    obj_t *obj;

    /* Add an int obj */
    obj = obj_pool_add_int(pool, 5);
    if(!obj){
        fprintf(stderr, "%s: Couldn't allocate int obj\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated int obj:\n", __func__);
    obj_dump(obj, stderr, 2);

    /* Add a sym obj */
    obj = obj_pool_add_sym(pool, string);
    if(!obj){
        fprintf(stderr, "%s: Couldn't allocate sym obj\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated sym obj:\n", __func__);
    obj_dump(obj, stderr, 2);

    /* Add a str obj */
    obj = obj_pool_add_str(pool, string);
    if(!obj){
        fprintf(stderr, "%s: Couldn't allocate str obj\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated str obj:\n", __func__);
    obj_dump(obj, stderr, 2);

    /* Add a nil obj */
    obj = obj_pool_add_nil(pool);
    if(!obj){
        fprintf(stderr, "%s: Couldn't allocate nil obj\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated nil obj:\n", __func__);
    obj_dump(obj, stderr, 2);

    /* Add a list of objs: (1 2 (3 "HALLO WARLD!") 99) */
    bool ok = false;
    do{
        obj_t *cell1;
        if(!(obj = cell1 = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_int(pool, 1)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_int(pool, 2)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        {
            obj_t *cell2;
            if(!(cell2 = OBJ_HEAD(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
            if(!(OBJ_HEAD(cell2) = obj_pool_add_int(pool, 3)))break;
            if(!(cell2 = OBJ_TAIL(cell2) = obj_pool_add_cell(pool, NULL, NULL)))break;
            if(!(OBJ_HEAD(cell2) = obj_pool_add_str(pool, string)))break;
            if(!(OBJ_TAIL(cell2) = obj_pool_add_nil(pool)))break;
        }
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_int(pool, 99)))break;
        if(!(OBJ_TAIL(cell1) = obj_pool_add_nil(pool)))break;
        ok = true;
    }while(0);
    if(!ok){
        fprintf(stderr, "%s: Couldn't allocate list of objs\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated list of objs:\n", __func__);
    obj_dump(obj, stderr, 2);

    return 0;
}

static int parse_buffer(
    obj_pool_t *pool, const char *filename,
    const char *buffer, size_t buffer_len
){
    fprintf(stderr, "Parsing file: %s\n", filename);
    obj_t *obj = obj_parse(pool, buffer, buffer_len);
    if(!obj){
        fprintf(stderr, "Couldn't parse file: %s\n", filename);
        return 1;
    }
    fprintf(stderr, "Parsed file: %s\n", filename);

    fprintf(stderr, "Resulting obj:\n");
    obj_dump(obj, stderr, 2);
    return 0;
}


int main(int n_args, char *args[]){

    if(n_args <= 1){
        print_help();
        return 1;
    }

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
            size_t buffer_len;
            char *buffer = load_file(arg, &buffer_len);
            if(!buffer)return 1;
            fprintf(stderr, "Loaded file: %s\n", arg);

            if(parse_buffer(pool, arg, buffer, buffer_len))return 1;
            free(buffer);
        }else if(!strcmp(arg, "-c")){
            if(i >= n_args - 1){
                fprintf(stderr, "Missing arg after %s\n", arg);
                return 1;
            }
            arg = args[++i];

            if(parse_buffer(pool, "<inline>", arg, strlen(arg)))return 1;
        }else if(!strcmp(arg, "-T")){
            fprintf(stderr, "Running obj test...\n");
            if(run_obj_test(pool)){
                fprintf(stderr, "*** Test failed! ***\n");
                return 1;
            }
            fprintf(stderr, "Test ok!\n");
        }else{
            fprintf(stderr, "Unrecognized option: %s\n", arg);
            return 1;
        }
    }

    obj_pool_dump(pool, stderr);

    obj_symtable_cleanup(table);
    obj_pool_cleanup(pool);

    fprintf(stderr, "OK!\n");
    return 0;
}
