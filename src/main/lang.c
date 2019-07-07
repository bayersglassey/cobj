
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../cobj.h"
#include "../lang.h"
#include "../utils.h"


static void print_help(){
    fprintf(stderr,
        "Arguments:\n"
        "  -f FILE    Loads, parses, & runs given file\n"
        "  -c TEXT    Parses & runs given text\n"
    );
}


static int parse_buffer(
    obj_vm_t *vm, const char *filename,
    const char *buffer, size_t buffer_len
){
    fprintf(stderr, "Parsing file: %s\n", filename);
    if(obj_vm_parse_raw(vm, filename, buffer, buffer_len)){
        fprintf(stderr, "Couldn't parse file: %s\n", filename);
        return 1;
    }
    fprintf(stderr, "Parsed file: %s\n", filename);
    return 0;
}


int main(int n_args, char *args[]){

    if(n_args <= 1){
        print_help();
        return 1;
    }

    obj_symtable_t _table, *table=&_table;
    obj_pool_t _pool, *pool=&_pool;
    obj_vm_t _vm, *vm=&_vm;

    obj_symtable_init(table);
    obj_pool_init(pool, table);
    obj_vm_init(vm, pool);

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

            if(parse_buffer(vm, arg, buffer, buffer_len))return 1;
            free(buffer);
        }else if(!strcmp(arg, "-c")){
            if(i >= n_args - 1){
                fprintf(stderr, "Missing arg after %s\n", arg);
                return 1;
            }
            arg = args[++i];

            if(parse_buffer(vm, "<inline>", arg, strlen(arg)))return 1;
        }else{
            fprintf(stderr, "Unrecognized option: %s\n", arg);
            return 1;
        }
    }

    obj_symtable_dump(table, stderr);
    obj_pool_dump(pool, stderr);
    obj_vm_dump(vm, stderr);

    obj_symtable_cleanup(table);
    obj_pool_cleanup(pool);
    obj_vm_cleanup(vm);

    fprintf(stderr, "OK!\n");
    return 0;
}
