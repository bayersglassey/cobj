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

