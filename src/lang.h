#ifndef _COBJ_LANG_H_
#define _COBJ_LANG_H_

#include "cobj.h"


typedef struct obj_vm obj_vm_t;


struct obj_vm {
    obj_pool_t *pool;
    obj_dict_t modules;
    obj_sym_t *sym_in;
    obj_sym_t *sym_from;
    obj_sym_t *sym_def;
    obj_sym_t *sym_arrow;
    /* !!!TODO: add obj_module_t and vm->module_list.
    Implementing modules as obj_t is "cool" but super
    limiting. */
};


void obj_vm_init(obj_vm_t *vm, obj_pool_t *pool){
    vm->pool = pool;
    obj_dict_init(&vm->modules);
}

void obj_vm_cleanup(obj_vm_t *vm){
    obj_dict_cleanup(&vm->modules);
}

void obj_vm_dump(obj_vm_t *vm, FILE *file){
    fprintf(file, "VM %p:\n", vm);
    obj_dict_t *modules = &vm->modules;
    fprintf(file, "  MODULES:");
    for(size_t i = 0; i < modules->entries_len; i++){
        obj_dict_entry_t *entry = &modules->entries[i];
        if(!entry->sym)continue;

        putc('\n', file);
        fprintf(file, "    ");

        obj_string_t *s = &entry->sym->string;
        fprintf(file, "%.*s", size_to_int(s->len, -1),
            s->data);

        putc(' ', file);
        _obj_dump((obj_t*)entry->value, file, 4);
    }
    putc('\n', file);
}

int obj_vm_get_syms(obj_vm_t *vm){
    vm->sym_in = obj_symtable_get_sym(vm->pool->symtable, "in");
    vm->sym_from = obj_symtable_get_sym(vm->pool->symtable, "from");
    vm->sym_def = obj_symtable_get_sym(vm->pool->symtable, "def");
    vm->sym_arrow = obj_symtable_get_sym(vm->pool->symtable, "->");
    if(!vm->sym_in || !vm->sym_from || !vm->sym_def || !vm->sym_arrow){
        fprintf(stderr,
            "%s: Couldn't get all syms! "
            "in=%p, from=%p, def=%p, arrow=%p\n",
            __func__,
            vm->sym_in, vm->sym_from, vm->sym_def, vm->sym_arrow);
        return 1;
    }
    return 0;
}

obj_t *obj_vm_get_module(obj_vm_t *vm, obj_sym_t *name){
    obj_t *module = obj_dict_get_value(&vm->modules, name);
    if(module)return module;

    /* Add module */
    obj_dict_t *defs = obj_pool_dict_alloc(vm->pool);
    if(!defs)return NULL;
    module = obj_pool_add_array(vm->pool, 2);
    if(!module)return NULL;
    obj_init_sym(OBJ_ARRAY_IGET(module, 0), name);
    obj_init_dict(OBJ_ARRAY_IGET(module, 1), defs);
    if(!obj_dict_set(&vm->modules, name, module))return NULL;
    return module;
}

obj_t *obj_vm_add_def(obj_vm_t *vm,
    obj_sym_t *name, obj_dict_t *scope, obj_t *body
){
    obj_t *def = obj_pool_add_array(vm->pool, 3);
    if(!def)return NULL;
    obj_init_sym(OBJ_ARRAY_IGET(def, 0), name);
    obj_init_dict(OBJ_ARRAY_IGET(def, 1), scope);
    obj_init_box(OBJ_ARRAY_IGET(def, 2), body);
    return def;
}

int obj_vm_parse_from(
    obj_vm_t *vm, obj_dict_t *scope,
    obj_sym_t *module_name, obj_t *body
){
    while(OBJ_TYPE(body) == OBJ_TYPE_CELL){
        obj_t *head = OBJ_HEAD(body);
        int head_type = OBJ_TYPE(head);
        if(head_type == OBJ_TYPE_SYM){
            obj_sym_t *head_sym = OBJ_SYM(head);
            obj_t *a = obj_pool_add_array(vm->pool, 2);
            if(!a)return 1;
            obj_init_sym(OBJ_ARRAY_IGET(a, 0), module_name);
            obj_init_sym(OBJ_ARRAY_IGET(a, 1), head_sym);
            if(!obj_dict_set(scope, head_sym, a))return 1;
        }else if(head_type == OBJ_TYPE_CELL){
            int head_len = OBJ_LIST_LEN(head);
            if(head_len != 3){
                fprintf(stderr, "%s: Expected list of length 3, got %i\n",
                    __func__, head_len);
                return 1;
            }
            obj_t *def_name_obj = OBJ_HEAD(head);
            head = OBJ_TAIL(head);
            obj_t *arrow_obj = OBJ_HEAD(head);
            head = OBJ_TAIL(head);
            obj_t *ref_name_obj = OBJ_HEAD(head);
            if(OBJ_TYPE(def_name_obj) != OBJ_TYPE_SYM){
                fprintf(stderr, "%s: Expected def_name type: SYM\n",
                    __func__);
                return 1;
            }
            if(
                OBJ_TYPE(arrow_obj) != OBJ_TYPE_SYM
                || OBJ_SYM(arrow_obj) != vm->sym_arrow
            ){
                fprintf(stderr, "%s: Expected sym: [->]\n",
                    __func__);
                return 1;
            }
            if(OBJ_TYPE(ref_name_obj) != OBJ_TYPE_SYM){
                fprintf(stderr, "%s: Expected ref_name type: SYM\n",
                    __func__);
                return 1;
            }

            obj_t *a = obj_pool_add_array(vm->pool, 2);
            if(!a)return 1;
            obj_init_sym(OBJ_ARRAY_IGET(a, 0), module_name);
            obj_init_sym(OBJ_ARRAY_IGET(a, 1), OBJ_SYM(def_name_obj));
            if(!obj_dict_set(scope, OBJ_SYM(ref_name_obj), a))return 1;
        }else{
            fprintf(stderr, "%s: Expected type: SYM or CELL\n", __func__);
            return 1;
        }
        body = OBJ_TAIL(body);
    }
    return 0;
}


int obj_vm_parse_raw(obj_vm_t *vm,
    const char *filename, const char *text, size_t text_len
){
#   define EXPECT(OBJ, TYPE) if(OBJ_TYPE(OBJ) != OBJ_TYPE_##TYPE){ \
        obj_parser_errmsg(parser, __func__); \
        fprintf(stderr, "Expected type: " #TYPE "\n"); \
        goto err; \
    }

    int status = 1;
    obj_parser_t _parser, *parser=&_parser;
    obj_parser_init(parser, vm->pool, filename, text, text_len);

    obj_t *code = obj_parser_parse(parser);
    if(!code)goto err;

    if(obj_vm_get_syms(vm))goto err;

    obj_sym_t *module_name = NULL;
    obj_t *module = NULL;
    obj_dict_t *scope = NULL;

    /* !!!NOTE: All cases of obj_parser_errmsg() in the following
    don't actually make sense, since parser has already finished parsing.
    So we need parser (or... pool?..) to provide a map from objs to token
    locations.
    So: a linked list of arrays of fixed *size* (e.g. 1024) & increasing
    *length* whose elements consist of an obj_t*, token_pos, token_col,
    token_row, etc.
    When matching an obj_t* against this structure, we may specify the
    "bounds" of our search, which are 2 pairs (one for start, one for end)
    of (pool chunk, offset).
    .....alternatively, we could use a hashmap (mapping obj_t* to token_pos,
    token_col, token_row, etc). */

    while(OBJ_TYPE(code) == OBJ_TYPE_CELL){
        obj_t *code_head = OBJ_HEAD(code);
        EXPECT(code_head, SYM)
        obj_sym_t *sym = OBJ_SYM(code_head);
        if(sym == vm->sym_in){
            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, SYM)
            module_name = OBJ_SYM(code_head);
            if(!module_name)goto err;
            fprintf(stderr, "-> Switched to module: %.*s\n",
                size_to_int(module_name->string.len, -1),
                module_name->string.data);
            module = obj_vm_get_module(vm, module_name);
            if(!module)goto err;
            scope = obj_pool_dict_alloc(vm->pool);
            if(!scope)goto err;
        }else if(sym == vm->sym_from){
            if(!module){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Illegal outside module.\n");
                goto err;
            }
            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, SYM)
            obj_sym_t *module_name = OBJ_SYM(code_head);
            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, CELL)
            if(obj_vm_parse_from(vm, scope, module_name, code_head))goto err;
        }else if(sym == vm->sym_def){
            if(!module){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Illegal outside module.\n");
                goto err;
            }
            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, SYM)
            obj_sym_t *def_name = OBJ_SYM(code_head);
            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, CELL)
            obj_dict_t *defs = OBJ_DICT(OBJ_ARRAY_IGET(module, 1));
            obj_t *def = obj_vm_add_def(vm, def_name, scope, code_head);
            if(!def)goto err;
            if(!obj_dict_set(defs, def_name, def))goto err;
        }else{
            obj_parser_errmsg(parser, __func__);
            fprintf(stderr, "Expected one of: in, from, def.\n");
            goto err;
        }
        code = OBJ_TAIL(code);
    }

    status = 0;
err:
    obj_parser_cleanup(parser);
    return status;
#   undef EXPECT
}

#endif