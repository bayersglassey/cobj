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

_OBJ_VM_MKSYM_SAME(dup)
_OBJ_VM_MKSYM_SAME(drop)
_OBJ_VM_MKSYM_SAME(swap)
_OBJ_VM_MKSYM_SAME(nip)
_OBJ_VM_MKSYM_SAME(tuck)
_OBJ_VM_MKSYM_SAME(over)

_OBJ_VM_MKSYM_SAME(p)
_OBJ_VM_MKSYM_SAME(p_stack)
_OBJ_VM_MKSYM_SAME(p_vars)
_OBJ_VM_MKSYM_SAME(p_blocks)
_OBJ_VM_MKSYM_SAME(p_frame)

_OBJ_VM_MKSYM_SAME(str_p)
