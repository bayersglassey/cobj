
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
        "  -f FILE        Loads & parses given file\n"
        "  -c TEXT        Parses given text\n"
        "  -m NAME        Finds given module\n"
        "  -d NAME        Finds given def within module found with -m\n"
        "  -p             Primes def found with -d (loads frame but doesn't run vm)\n"
        "  -e             Executes def found with -d\n"
        "  -D             Dumps symtable, pool, and vm\n"
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

    obj_sym_t *sym_empty = obj_symtable_get_sym(table, "");
    if(!sym_empty)return 1;
    obj_t *cur_module = obj_vm_get_or_add_module(vm, sym_empty);
    if(!cur_module)return 1;
    obj_t *cur_def = NULL;
    bool executed = false;

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
        }else if(!strcmp(arg, "-m")){
            if(i >= n_args - 1){
                fprintf(stderr, "Missing arg after %s\n", arg);
                return 1;
            }
            arg = args[++i];
            obj_sym_t *module_name = obj_symtable_get_sym(table, arg);

            cur_module = obj_vm_get_module(vm, module_name);
            if(!cur_module){
                fprintf(stderr, "Couldn't find module: ");
                obj_sym_fprint(module_name, stderr);
                putc('\n', stderr);
                return 1;
            }
        }else if(!strcmp(arg, "-d")){
            if(i >= n_args - 1){
                fprintf(stderr, "Missing arg after %s\n", arg);
                return 1;
            }
            arg = args[++i];
            obj_sym_t *def_name = obj_symtable_get_sym(table, arg);

            cur_def = cur_module?
                obj_module_get_def(cur_module, def_name): NULL;
            if(!cur_def){
                fprintf(stderr, "Couldn't find def: ");
                obj_sym_fprint(def_name, stderr);
                if(!cur_module){
                    fprintf(stderr, " - no module loaded!");
                }
                putc('\n', stderr);
                return 1;
            }
        }else if(!strcmp(arg, "-D")){
            obj_symtable_dump(table, stderr);
            obj_pool_dump(pool, stderr);
            obj_vm_dump(vm, stderr);
        }else if(!strcmp(arg, "-p") || !strcmp(arg, "-e")){
            if(!cur_def){
                fprintf(stderr, "No def loaded!\n");
                return 1;
            }
            fprintf(stderr, "Executing def: @@ ");
            obj_sym_fprint(OBJ_MODULE_GET_NAME(cur_module), stderr);
            putc(' ', stderr);
            obj_sym_fprint(OBJ_DEF_GET_NAME(cur_def), stderr);
            putc('\n', stderr);
            executed = true;

            if(!obj_vm_push_frame(vm, cur_module, cur_def))return 1;
            if(!strcmp(arg, "-e")){
                if(obj_vm_run(vm))return 1;
            }
        }else{
            fprintf(stderr, "Unrecognized option: %s\n", arg);
            return 1;
        }
    }

    if(!executed){
        if(cur_def){
            fprintf(stderr, "LOADED DEF:\n");
            obj_dump(cur_def, stderr, 2);
        }else if(cur_module){
            fprintf(stderr, "LOADED MODULE:\n");
            obj_dump(cur_module, stderr, 2);
        }else{
            fprintf(stderr, "NOTHING LOADED\n");
        }
    }

    obj_symtable_cleanup(table);
    obj_pool_cleanup(pool);
    obj_vm_cleanup(vm);

    fprintf(stderr, "OK!\n");
    return 0;
}
