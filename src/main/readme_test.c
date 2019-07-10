#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "../cobj.h"


int main(){
    /*** The example code from README.md ***/


    /* It's up to you to load the text from a file or whatnot. */
    /* This is C, for goodness' sake, not Python. */
    const char *text =
        "stuff:\n"
        "    ints: 1 2 3\n"
        "    strings: \"A\" \"B\" \"C\""
    ;

    /* Need to initialize a symtable and pool */
    obj_symtable_t table;
    obj_pool_t pool;
    obj_symtable_init(&table);
    obj_pool_init(&pool, &table);

    /* Parse the text as a list of values (a.k.a. a value of type list) */
    obj_t *obj = obj_parse(&pool, "<test data>", text, strlen(text));

    /* If there was an error, panic and review your stderr */
    assert(obj);

    /* Pretty-print the value */
    obj_dump(obj, stdout, 0);

    /* Top-level value returned by parser is always a list */
    assert(OBJ_TYPE(obj) == OBJ_TYPE_CELL);

    /* Remember the "stuff" symbol in the text? */
    obj_t *stuff = OBJ_HEAD(obj);
    assert(OBJ_TYPE(stuff) == OBJ_TYPE_SYM);
    obj_sym_t *sym = OBJ_SYM(stuff);
    /* The following prints "stuff" and a newline: */
    printf("%.*s\n", (int)sym->string.len, sym->string.data);

    /* Jump to next node of the top-level list */
    obj = OBJ_TAIL(obj);
    assert(OBJ_TYPE(obj) == OBJ_TYPE_CELL);

    /* The list of "stuff" */
    obj_t *stuff_list = OBJ_HEAD(obj);
    assert(OBJ_LEN(stuff_list) == 4);

    /* OBJ_IGET gets values in the list by integer index */
    obj_t *ints_sym = OBJ_IGET(stuff_list, 0); /* The "ints" symbol in the text */
    obj_t *ints_list = OBJ_IGET(stuff_list, 1); /* The list of integers in the text */
    obj_t *strings_sym = OBJ_IGET(stuff_list, 2); /* The "strings" symbol in the text */
    obj_t *strings_list = OBJ_IGET(stuff_list, 3); /* The list of strings in the text */

    /* OBJ_GET gets values in the list by symbol lookup. */
    /* This interprets the list as a series of key/value pairs. */
    /* You must first get/create the symbols in the symbol table. */
    obj_sym_t *SYM_ints = obj_symtable_get_sym(&table, "ints");
    obj_sym_t *SYM_strings = obj_symtable_get_sym(&table, "strings");
    obj_sym_t *SYM_blabla = obj_symtable_get_sym(&table, "blabla");
    assert(OBJ_GET(stuff_list, SYM_ints) == ints_list);
    assert(OBJ_GET(stuff_list, SYM_strings) == strings_list);
    assert(OBJ_GET(stuff_list, SYM_blabla) == NULL);

    /* Jump to next node of the top-level list... but it's nil, marking the end of the list */
    obj = OBJ_TAIL(obj);
    assert(OBJ_TYPE(obj) == OBJ_TYPE_NIL);

    /* Free everything */
    obj_pool_cleanup(&pool);
    obj_symtable_cleanup(&table);

    return 0;
}
