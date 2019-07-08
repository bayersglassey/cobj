#ifndef _COBJ_LANG_H_
#define _COBJ_LANG_H_

#include "cobj.h"

typedef struct obj_vm obj_vm_t;


struct obj_vm {
    obj_pool_t *pool;
    obj_dict_t modules;
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


int obj_vm_parse_raw(obj_vm_t *vm,
    const char *filename, const char *text, size_t text_len
){
    int status = 1;
    obj_parser_t _parser, *parser=&_parser;
    obj_parser_init(parser, vm->pool, filename, text, text_len);

    obj_t *code = obj_parser_parse(parser);
    if(!code)goto err;

    obj_sym_t *sym_in = obj_symtable_get_sym(
        vm->pool->symtable, "in");
    obj_sym_t *sym_from = obj_symtable_get_sym(
        vm->pool->symtable, "from");
    obj_sym_t *sym_def = obj_symtable_get_sym(
        vm->pool->symtable, "def");
    if(!sym_in || !sym_from || !sym_def)goto err;

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
        if(OBJ_TYPE(code_head) != OBJ_TYPE_SYM){
            obj_parser_errmsg(parser, __func__);
            fprintf(stderr, "Expected: sym\n");
            goto err;
        }
        obj_sym_t *sym = OBJ_SYM(code_head);
        if(sym == sym_in){
            code = OBJ_TAIL(code);
            if(OBJ_TYPE(code) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
            code_head = OBJ_HEAD(code);
            if(OBJ_TYPE(code_head) != OBJ_TYPE_SYM){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: sym\n");
                goto err;
            }
            module_name = OBJ_SYM(code_head);
            if(!module_name)goto err;
            fprintf(stderr, "-> Switched to module: %.*s\n",
                size_to_int(module_name->string.len, -1),
                module_name->string.data);
            module = obj_vm_get_module(vm, module_name);
            if(!module)goto err;
            scope = obj_pool_dict_alloc(vm->pool);
            if(!scope)goto err;
        }else if(sym == sym_from){
            if(!module){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Illegal outside module.\n");
                goto err;
            }
            code = OBJ_TAIL(code);
            if(OBJ_TYPE(code) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
            code_head = OBJ_HEAD(code);
            if(OBJ_TYPE(code_head) != OBJ_TYPE_SYM){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: sym\n");
                goto err;
            }
            obj_sym_t *ref_name = OBJ_SYM(code_head);
            code = OBJ_TAIL(code);
            if(OBJ_TYPE(code) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
            code_head = OBJ_HEAD(code);
            if(OBJ_TYPE(code_head) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
            if(!obj_dict_set(scope, ref_name, code_head))goto err;
        }else if(sym == sym_def){
            if(!module){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Illegal outside module.\n");
                goto err;
            }
            code = OBJ_TAIL(code);
            if(OBJ_TYPE(code) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
            code_head = OBJ_HEAD(code);
            if(OBJ_TYPE(code_head) != OBJ_TYPE_SYM){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: sym\n");
                goto err;
            }
            obj_sym_t *def_name = OBJ_SYM(code_head);
            code = OBJ_TAIL(code);
            if(OBJ_TYPE(code) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
            code_head = OBJ_HEAD(code);
            if(OBJ_TYPE(code_head) != OBJ_TYPE_CELL){
                obj_parser_errmsg(parser, __func__);
                fprintf(stderr, "Expected: cell\n");
                goto err;
            }
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
}

#endif