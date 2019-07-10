# COBJ (C objects)


## What is this

Small C library for nested data structures made up of the following types of value:

* Integer

* Symbol

* String

* List

These values are represented by small tagged unions (type `obj_t`).
Memory is managed by pool structures (type `obj_pool_t`) which preallocate large arrays of `obj_t` and keep track of strings.
Pools can also share symbol tables (type `obj_symtable_t`).

Allocating values is fast. There is no reference counting or other GC bookkeeping.
Each memory pool can be almost instantly freed.

The intended use case is:

* Build up a large data structure (e.g. parse a file containing hierarchical data)

* Iterate over the data structure (e.g. rendering it, or doing some kind of analysis)

* Destroy the data structure and start over with fresh data


## Text format

It's a flavour of S-expression.


Comments:

    # Comments start with the hash symbol, "#".


Integers:

    1
    2000
    -3

    12.5
    # No decimals! This is the same as: 12  .  5
    # See "operators" below.


Symbols:

    # Symbols come in three flavours: names, operators, and freeform.
    # The distinction is only important when lexing (splitting up text into tokens).
    # Names are the usual alphanumeric + underscore, with no leading digits.
    # Operators are runs of anything other than digits, parentheses, brackets, colons, and quotes.
    # Freeform symbols are surrounded by brackets: [...]


    ############
    # Names:

    x
    x2
    mySymbol
    __
    ASD_34x__zz

    2x
    # Same as: 2  x


    ##############
    # Operators:

    +
    -
    ++
    +-+
    ==>
    $$-
    ........!%!$%^!$%&!$....

    .x
    # Same as: .  x

    --2
    # Same as: --  2

    ^%_%^
    # Same as: ^%  _  %^


    ###########
    # Freeform:

    [A long symbol]
    [a.short.symbol]
    [/usr/bin/]
    [I contain \[escaped brackets\]]
    [I contain "quotes" and\na newline]
    [Here is one backslash: \\]

    [x]
    # Same as: x


    ###########
    # Mixed:

    x.y
    # Same as: x  .  y

    i+1
    # Same as: i  +  1

    @F.[x.y]
    # Same as: @  F  .  [x.y]


Strings:

    "x"
    "abc 123"
    "I am \"quoted\""
    "I AM\nON SEPARATE\nLINES"
    "Here is a backslash: \\"


    ####################################################
    # There are also full-line strings, which are handy
    # if you want to freely use "#", "\", etc.

    ;This is a "full-line" string. \o/  # I am not a comment!
    # Same as:
    # "This is a \"full-line\" string. \\o/  # I am not a comment!"


    ####################################################
    # There is no way to get the newline character
    # inside a full-line string.
    # But you can make a list of full-line strings.

    :
        ;Here's a paragraph of text.
        ;Actually, it's a list of strings,
        ;but if you joined them together
        ;separated by newlines...
        ;you get the idea.


Lists:

    # Lists can use parentheses or colons+indentation.
    # The two styles can be mixed.


    ##################################
    # The following are equivalent

    (1 2 3)

    : 1 2 3


    ##################################
    # The following are equivalent

    (1 ("two" three) 4)

    : 1 ("two" three) 4

    : 1
        : "two" three
        4


    ##################################
    # The following are equivalent

    stuff:
        ints: 1 2 3
        strings: "A" "B" "C"

    stuff (ints (1 2 3) strings ("A" "B" "C"))


    ##################################
    # The following are equivalent

    (x: 23)

    (x (23))


## Extended data types

Lists are great, but when actually writing a program, you usually
want a couple of other native data structures with better performance.

The text format can be extended in a backwards-compatible way to
support various datatypes using a special "typecast" prefix (the
datatype's name in curly brackets) and a list.

Arrays:

    # If extended data types are activated, the following is parsed
    # as an array.
    # Otherwise, it's parsed as the list (1 2 3).
    # Arrays can only directly store values which can be represented
    # as a single obj_t (integer, symbol, string, dict, box).
    # Boxes can be used to indirectly store values of any type.
    # Arrays have constant-time index-based lookup and update.
    # They cannot be resized.

    {array}: 1 2 3

Dicts:

    # If extended data types are activated, the following is parsed
    # as a dict.
    # Otherwise, it's parsed as the list (x 1 y 2).
    # Dicts map symbols to arbitrary values.
    # They have more-or-less constant-time key-based lookup and update.
    # Key-value mappings may be freely added and removed.

    {dict}:
        x 1
        y 2

Structs:

    # If extended data types are activated, the following is parsed
    # as a struct.
    # Otherwise, it's parsed as the list (x 1 y 2).
    # Structs map symbols to values which can be represented
    # as a single obj_t (integer, symbol, string, dict, box).
    # Boxes can be used to indirectly store values of any type.
    # Structs have linear-time key-based lookup and update.
    # They cannot be resized.

    {struct}:
        x 1
        y 2


## C interface

Parse some text:

    #include <stdio.h>
    #include <string.h>
    #include "cobj.h"

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
    obj_t *obj = obj_parse(&pool, text, strlen(text));

    /* If there was an error, panic and review your stderr */
    if(!obj)exit(1);

    /* Pretty-print the value */
    obj_dump(stdout, obj, 0);

    /* Top-level value returned by parser is always a list */
    assert(OBJ_TYPE(obj) == OBJ_TYPE_CELL_HEAD);

    /* Remember the "stuff" symbol in the text? */
    obj_t *stuff = OBJ_HEAD(obj);
    assert(OBJ_TYPE(stuff) == OBJ_TYPE_SYM);
    obj_string_t *string = OBJ_STRING(stuff);
    printf("%.*s", (int)string.len, string.data); /* Prints "stuff" */

    /* Jump to next node of the top-level list */
    obj = OBJ_TAIL(obj);
    assert(OBJ_TYPE(obj) == OBJ_TYPE_CELL_HEAD);

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
    /* You must first get/create the symbols as strings in the symbol table. */
    obj_string_t *SYM_ints = obj_symtable_get(&table, "ints");
    obj_string_t *SYM_strings = obj_symtable_get(&table, "strings");
    obj_string_t *SYM_blabla = obj_symtable_get(&table, "blabla");
    assert(OBJ_GET(obj, SYM_ints) == ints_list);
    assert(OBJ_GET(obj, SYM_strings) == strings_list);
    assert(OBJ_GET(obj, SYM_blabla) == NULL);

    /* Jump to next node of the top-level list... but it's nil, marking the end of the list */
    obj = OBJ_TAIL(obj);
    assert(OBJ_TYPE(obj) == OBJ_TYPE_NIL);

    /* Free everything */
    obj_pool_cleanup(&pool);
    obj_symtable_cleanup(&table);
