
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../cobj.h"


static int run_obj_test(){
    obj_symtable_t _table, *table=&_table;
    obj_pool_t _pool, *pool=&_pool;
    obj_symtable_init(table);
    obj_pool_init(pool, table);

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

    obj_pool_dump(pool, stderr);
    obj_symtable_cleanup(table);
    obj_pool_cleanup(pool);
    return 0;
}


int main(int n_args, char *args[]){

    fprintf(stderr, "Running obj test...\n");
    if(run_obj_test()){
        fprintf(stderr, "*** Test failed! ***\n");
        return 1;
    }
    fprintf(stderr, "Test ok!\n");

    fprintf(stderr, "OK!\n");
    return 0;
}
