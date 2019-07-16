/* This file is expected to be #included with various #definitions of _OBJ_VM_MKSYM */

#define _OBJ_VM_MKSYM_SAME(NAME) _OBJ_VM_MKSYM(NAME, #NAME)

_OBJ_VM_MKSYM_SAME(module)
_OBJ_VM_MKSYM_SAME(from)
_OBJ_VM_MKSYM_SAME(def)
_OBJ_VM_MKSYM_SAME(in)
_OBJ_VM_MKSYM_SAME(out)
_OBJ_VM_MKSYM_SAME(docs)

_OBJ_VM_MKSYM(arrow, "->")
_OBJ_VM_MKSYM(var_get, "'")
_OBJ_VM_MKSYM(var_set, "='")
_OBJ_VM_MKSYM(struct_get, ".")
_OBJ_VM_MKSYM(struct_set, "=.")

_OBJ_VM_MKSYM(add, "+")
_OBJ_VM_MKSYM(sub, "-")
_OBJ_VM_MKSYM(mul, "*")
_OBJ_VM_MKSYM(div, "/")
_OBJ_VM_MKSYM_SAME(mod)
_OBJ_VM_MKSYM(eq, "==")
_OBJ_VM_MKSYM(ne, "!=")
_OBJ_VM_MKSYM(lt, "<")
_OBJ_VM_MKSYM(le, "<=")
_OBJ_VM_MKSYM(gt, ">")
_OBJ_VM_MKSYM(ge, ">=")

_OBJ_VM_MKSYM(sym_lit, "`")
_OBJ_VM_MKSYM_SAME(T)
_OBJ_VM_MKSYM_SAME(F)
_OBJ_VM_MKSYM_SAME(not)
_OBJ_VM_MKSYM_SAME(if)
_OBJ_VM_MKSYM_SAME(ifelse)

_OBJ_VM_MKSYM_SAME(dup)
_OBJ_VM_MKSYM_SAME(drop)
_OBJ_VM_MKSYM_SAME(swap)
_OBJ_VM_MKSYM_SAME(nip)
_OBJ_VM_MKSYM_SAME(tuck)
_OBJ_VM_MKSYM_SAME(over)

_OBJ_VM_MKSYM(call, "@")
_OBJ_VM_MKSYM(longcall, "@@")

_OBJ_VM_MKSYM_SAME(struct)
_OBJ_VM_MKSYM_SAME(dict)
_OBJ_VM_MKSYM_SAME(arr)
_OBJ_VM_MKSYM_SAME(get)
_OBJ_VM_MKSYM_SAME(set)
_OBJ_VM_MKSYM_SAME(len)
_OBJ_VM_MKSYM(arr_get, ".$")
_OBJ_VM_MKSYM(arr_set, "=.$")

_OBJ_VM_MKSYM_SAME(assert)

_OBJ_VM_MKSYM_SAME(p)
_OBJ_VM_MKSYM_SAME(p_stack)
_OBJ_VM_MKSYM_SAME(p_vars)
_OBJ_VM_MKSYM_SAME(p_blocks)
_OBJ_VM_MKSYM_SAME(p_frame)

_OBJ_VM_MKSYM_SAME(str_p)