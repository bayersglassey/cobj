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

    # Symbols come in two flavours: names and operators.
    # Names are the usual alphanumeric + underscore, with no leading digits.
    # Operators are runs of anything other than digits, parentheses, colons, and quotes.


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
    # Mixed:

    x.y
    # Same as: x  .  y

    i+1
    # Same as: i  +  1


Strings:

    "x"
    "abc 123"
    "I am \"quoted\""
    "I AM\nON SEPARATE\nLINES"


Lists:

    # Lists can use parentheses or colons+newlines.
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


