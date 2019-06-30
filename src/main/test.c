
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
    bool ok;

    /* Add a symbol to the symtable */
    obj_sym_t *sym = obj_symtable_get_sym(table, "xyz_321");
    if(!sym){
        fprintf(stderr, "%s: Couldn't allocate sym\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated sym: %.*s (hash=%zu)\n",
        __func__, (int)sym->string.len, sym->string.data, sym->hash);

    obj_sym_t *same_sym = obj_symtable_get_sym(table, "xyz_321");
    if(same_sym != sym){
        fprintf(stderr, "%s: Syms were not the same...\n", __func__);
        return 1;
    }

    obj_sym_t *other_sym = obj_symtable_get_sym(table, "lalala");
    if(other_sym == NULL){
        fprintf(stderr, "%s: Couldn't allocate other sym\n", __func__);
        return 1;
    }

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
    obj = obj_pool_add_sym(pool, sym);
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
    ok = false;
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
    {
        obj_t *elem1 = OBJ_IGET(obj, 1);
        if(OBJ_TYPE(elem1) != OBJ_TYPE_INT || elem1->u.i != 2){
            fprintf(stderr, "%s: Element 1: wrong value\n", __func__);
            return 1;
        }

        obj_t *elem2_0 = OBJ_IGET(OBJ_IGET(obj, 2), 0);
        if(OBJ_TYPE(elem2_0) != OBJ_TYPE_INT || elem2_0->u.i != 3){
            fprintf(stderr, "%s: Element 2, 0: wrong value\n", __func__);
            return 1;
        }

        obj_t *elem4 = OBJ_IGET(obj, 4);
        if(elem4 != NULL){
            fprintf(stderr, "%s: Element 4: unexpectedly found\n", __func__);
            return 1;
        }
    }

    /* Add a list of objs: (x 10 y 20 x 30) */
    ok = false;
    obj_sym_t *sym_x = obj_symtable_get_sym(table, "x");
    obj_sym_t *sym_y = obj_symtable_get_sym(table, "y");
    if(!sym_x || !sym_y){
        fprintf(stderr, "%s: Couldn't allocate sym (x or y)\n", __func__);
        return 1;
    }
    do{
        obj_t *cell1;
        if(!(obj = cell1 = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_sym(pool, sym_x)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_int(pool, 10)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_sym(pool, sym_y)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_int(pool, 20)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_sym(pool, sym_x)))break;
        if(!(cell1 = OBJ_TAIL(cell1) = obj_pool_add_cell(pool, NULL, NULL)))break;
        if(!(OBJ_HEAD(cell1) = obj_pool_add_int(pool, 30)))break;
        if(!(OBJ_TAIL(cell1) = obj_pool_add_nil(pool)))break;
        ok = true;
    }while(0);
    if(!ok){
        fprintf(stderr, "%s: Couldn't allocate list of objs\n", __func__);
        return 1;
    }
    fprintf(stderr, "%s: Allocated list of objs:\n", __func__);
    obj_dump(obj, stderr, 2);
    {
        obj_t *elem_x = OBJ_GET(obj, sym_x);
        if(OBJ_TYPE(elem_x) != OBJ_TYPE_INT || elem_x->u.i != 10){
            fprintf(stderr, "%s: Element x: wrong value\n", __func__);
            return 1;
        }

        obj_t *elem_y = OBJ_GET(obj, sym_y);
        if(OBJ_TYPE(elem_y) != OBJ_TYPE_INT || elem_y->u.i != 20){
            fprintf(stderr, "%s: Element y: wrong value\n", __func__);
            return 1;
        }
    }

    obj_symtable_dump(table, stderr);
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
