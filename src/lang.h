#ifndef _COBJ_LANG_H_
#define _COBJ_LANG_H_

#include "cobj.h"


#define OBJ_FRAME_DEFAULT_VARS_LEN 8
#define OBJ_FRAME_DEFAULT_STACK_LEN 8

/* TOS: Top Of Stack, NOS: Next On Stack, 3OS: Third On Stack */
#define OBJ_FRAME_TOS(frame) &frame->stack[frame->stack_tos - 1]
#define OBJ_FRAME_NOS(frame) &frame->stack[frame->stack_tos - 2]
#define OBJ_FRAME_3OS(frame) &frame->stack[frame->stack_tos - 3]

#define OBJ_MODULE_GET_NAME(module) OBJ_SYM(OBJ_ARRAY_IGET(module, 0))
#define OBJ_MODULE_GET_DEFS(module) OBJ_DICT(OBJ_ARRAY_IGET(module, 1))

#define OBJ_DEF_GET_NAME(def) OBJ_SYM(OBJ_ARRAY_IGET(def, 0))
#define OBJ_DEF_GET_SCOPE(def) OBJ_DICT(OBJ_ARRAY_IGET(def, 1))
#define OBJ_DEF_GET_N_ARGS(def) OBJ_INT(OBJ_ARRAY_IGET(def, 2))
#define OBJ_DEF_GET_N_RETS(def) OBJ_INT(OBJ_ARRAY_IGET(def, 3))
#define OBJ_DEF_GET_ARGS(def) OBJ_CONTENTS(OBJ_ARRAY_IGET(def, 4))
#define OBJ_DEF_GET_RETS(def) OBJ_CONTENTS(OBJ_ARRAY_IGET(def, 5))
#define OBJ_DEF_GET_CODE(def) OBJ_CONTENTS(OBJ_ARRAY_IGET(def, 6))

#define OBJ_REF_GET_MODULE_NAME(ref) OBJ_SYM(OBJ_ARRAY_IGET(ref, 0))
#define OBJ_REF_GET_DEF_NAME(ref) OBJ_SYM(OBJ_ARRAY_IGET(ref, 1))


typedef struct obj_vm obj_vm_t;
typedef struct obj_frame obj_frame_t;
typedef struct obj_block obj_block_t;


struct obj_block {
    obj_block_t *next;

    obj_t *code;
        /* code: OBJ_TYPE_CELL or OBJ_TYPE_NIL */
};

struct obj_frame {
    obj_frame_t *next;

    obj_t *module;
    obj_t *def;
        /* module: OBJ_TYPE_ARRAY */
        /* def: OBJ_TYPE_ARRAY */

    size_t vars_len;
    size_t n_vars;
    obj_t *vars;
        /* vars_len: length of memory allocated for vars */
        /* n_vars: number of vars (key+value pairs) */
        /* vars: array of obj_t*, alternating keys (OBJ_TYPE_SYM) and
        values (of arbitrary types) */

    size_t stack_len;
    size_t stack_tos;
    obj_t *stack;
        /* stack_len: size of memory allocated for stack */
        /* stack_tos: index of top of stack */
        /* stack: array of obj_t */

    int n_blocks;
    obj_block_t *block_list;
        /* n_blocks: counter, incremented and decremented as blocks
        are pushed & popped */
        /* block_list: see vm->free_block_list */
};

struct obj_vm {
    obj_pool_t *pool;
    obj_dict_t modules;

    int n_frames;
    obj_frame_t *frame_list;
    obj_frame_t *free_frame_list;
        /* n_frames: counter, incremented and decremented as frames
        are pushed & popped */
        /* free_frame_list is a linked list of preallocated
        frames.
        So when we pop from vm->frame_list, instead of freeing,
        we push onto vm->free_frame_list.
        Then when we push to vm->frame_list, we pop from
        vm->free_frame_list if available, only otherwise do we
        malloc. */

    obj_block_t *free_block_list;
        /* free_block_list is a linked list of preallocated
        blocks, for use by the frames in vm->frame_list.
        So when we pop from frame->block_list, instead of freeing,
        we push onto vm->free_block_list.
        Then when we push to frame->block_list, we pop from
        vm->free_block_list if available, only otherwise do we
        malloc. */

    #define _OBJ_VM_MKSYM(NAME, STRING) obj_sym_t *sym_##NAME;
    #include "vm_mksym.inc"
    #undef _OBJ_VM_MKSYM

};



/************
* obj_block *
************/

void obj_block_init(obj_block_t *block, obj_block_t *next, obj_t *code){
    memset(block, 0, sizeof(*block));
    block->next = next;
    block->code = code;
}

void obj_block_cleanup(obj_block_t *block){
    while(block){
        obj_block_t *next = block->next;
        free(block);
        block = next;
    }
}



/************
* obj_frame *
************/

void obj_frame_init(
    obj_frame_t *frame, obj_frame_t *next, obj_t *module, obj_t *def
){
    /* NOTE: we do NOT zero frame's memory. The entries of
    vm->free_frame_list retain their allocated vars and stack, so that
    obj_vm_push_frame can avoid allocating memory at all if the program
    has been running long enough. */
    frame->next = next;
    frame->module = module;
    frame->def = def;
    frame->n_vars = 0;
    frame->stack_tos = 0;
}

void obj_frame_cleanup(obj_frame_t *frame){
    while(frame){
        obj_frame_t *next = frame->next;
        free(frame->vars);
        free(frame->stack);
        obj_block_cleanup(frame->block_list);
        free(frame);
        frame = next;
    }
}

void obj_frame_dump_stack(obj_frame_t *frame, FILE *file, int depth){
    _print_tabs(file, depth);
    fprintf(file, "STACK (%zu/%zu):\n",
        frame->stack_tos, frame->stack_len);
    for(size_t i = 0; i < frame->stack_tos; i++){
        obj_t *obj = &frame->stack[i];
        obj_dump(obj, file, depth + 2);
    }
}

void obj_frame_dump_vars(obj_frame_t *frame, FILE *file, int depth){
    _print_tabs(file, depth);
    fprintf(file, "VARS (%zu*2=%zu/%zu):\n",
        frame->n_vars, frame->n_vars * 2, frame->vars_len);
    for(size_t i = 0; i < frame->n_vars; i++){
        obj_t *key_obj = &frame->vars[i * 2];
        obj_t *val_obj = &frame->vars[i * 2 + 1];
        _print_tabs(file, depth + 2);
        obj_sym_fprint(OBJ_SYM(key_obj), file);
        putc(' ', file);
        obj_fprint(val_obj, file, depth + 2);
        putc('\n', file);
    }
}

void obj_frame_dump_blocks(obj_frame_t *frame, FILE *file, int depth){
    _print_tabs(file, depth);
    fprintf(file, "BLOCKS (%i):\n", frame->n_blocks);
    for(obj_block_t *block = frame->block_list;
        block; block = block->next
    ){
        _print_tabs(file, depth);
        fprintf(file, "  BLOCK %p:\n", block);
        obj_dump(block->code, file, depth + 4);
    }
}

void obj_frame_dump(obj_frame_t *frame, FILE *file, int depth){
    _print_tabs(file, depth);
    fprintf(file, "FRAME %p:\n", frame);
    _print_tabs(file, depth);
    fprintf(file, "  DEF:\n");
    obj_dump(frame->def, file, depth + 4);
    obj_frame_dump_stack(frame, file, depth + 2);
    obj_frame_dump_vars(frame, file, depth + 2);
    obj_frame_dump_blocks(frame, file, depth + 2);
}

obj_t *obj_frame_push(obj_frame_t *frame, obj_t *obj){
    if(frame->stack_tos >= frame->stack_len){
        size_t stack_len = !frame->stack_len?
            OBJ_FRAME_DEFAULT_STACK_LEN: frame->stack_len * 2;
        obj_t *stack = realloc(frame->stack, stack_len * sizeof(*stack));
        if(!stack){
            fprintf(stderr,
                "%s: Couldn't allocate %zu byte stack. ",
                    __func__, stack_len);
            perror("realloc");
            return NULL;
        }
        frame->stack = stack;
        frame->stack_len = stack_len;
    }
    frame->stack[frame->stack_tos] = *obj;
    obj_t *return_obj = &frame->stack[frame->stack_tos];
    frame->stack_tos++;
    return return_obj;
}

obj_t *obj_frame_get_var(obj_frame_t *frame, obj_sym_t *sym){
    for(size_t i = 0; i < frame->n_vars; i++){
        if(OBJ_SYM(&frame->vars[i * 2]) == sym){
            return &frame->vars[i * 2 + 1];
        }
    }
    return NULL;
}

obj_t *obj_frame_add_var(obj_frame_t *frame, obj_sym_t *sym){
    if(frame->n_vars * 2 >= frame->vars_len){
        size_t vars_len = !frame->vars_len?
            OBJ_FRAME_DEFAULT_VARS_LEN: frame->vars_len * 2;
        obj_t *vars = realloc(frame->vars, vars_len * sizeof(*vars));
        if(!vars){
            fprintf(stderr,
                "%s: Couldn't allocate %zu byte vars. ",
                    __func__, vars_len);
            perror("realloc");
            return NULL;
        }
        frame->vars = vars;
        frame->vars_len = vars_len;
    }
    size_t i = frame->n_vars * 2;
    frame->n_vars++;
    obj_init_sym(&frame->vars[i], sym);
    obj_init_null(&frame->vars[i + 1]);
    return &frame->vars[i + 1];
}

obj_t *obj_frame_get_or_add_var(obj_frame_t *frame, obj_sym_t *sym){
    obj_t *var = obj_frame_get_var(frame, sym);
    if(var)return var;
    return obj_frame_add_var(frame, sym);
}

obj_t *obj_frame_set_var(obj_frame_t *frame, obj_sym_t *sym, obj_t *obj){
    obj_t *var = obj_frame_get_or_add_var(frame, sym);
    if(!var)return NULL;
    *var = *obj;
    return var;
}

obj_block_t *obj_frame_push_block(
    obj_vm_t *vm, obj_frame_t *frame, obj_t *code
){
    obj_block_t *block;
    if(vm->free_block_list){
        block = vm->free_block_list;
        vm->free_block_list = block->next;
    }else{
        block = calloc(sizeof(*block), 1);
        if(!block)return NULL;
    }
    obj_block_init(block, frame->block_list, code);
    frame->block_list = block;
    frame->n_blocks++;
    return block;
}

obj_block_t *obj_frame_pop_block(obj_vm_t *vm, obj_frame_t *frame){
    obj_block_t *block = frame->block_list;
    if(!block)return NULL;
    frame->block_list = block->next;
    block->next = vm->free_block_list;
    vm->free_block_list = block;
    frame->n_blocks--;
    return block;
}


/*********
* obj_vm *
*********/

void obj_vm_init(obj_vm_t *vm, obj_pool_t *pool){
    memset(vm, 0, sizeof(*vm));
    vm->pool = pool;
    obj_dict_init(&vm->modules);
}

void obj_vm_cleanup(obj_vm_t *vm){
    obj_dict_cleanup(&vm->modules);
    obj_frame_cleanup(vm->frame_list);
    obj_frame_cleanup(vm->free_frame_list);
    obj_block_cleanup(vm->free_block_list);
}

void obj_vm_dump_modules(obj_vm_t *vm, FILE *file, int depth){
    obj_dict_t *modules = &vm->modules;
    _print_tabs(file, depth);
    fprintf(file, "MODULES:");
    obj_dict_fprint(&vm->modules, file, depth + 2);
    putc('\n', file);
}

void obj_vm_dump_frames(obj_vm_t *vm, FILE *file, int depth){
    _print_tabs(file, depth);
    fprintf(file, "FRAMES (%i):\n", vm->n_frames);
    for(obj_frame_t *frame = vm->frame_list;
        frame; frame = frame->next
    ){
        obj_frame_dump(frame, file, depth + 2);
    }
}

void obj_vm_dump(obj_vm_t *vm, FILE *file){
    fprintf(file, "VM %p:\n", vm);
    obj_vm_dump_modules(vm, file, 2);
    obj_vm_dump_frames(vm, file, 2);
}

int obj_vm_get_syms(obj_vm_t *vm){
    /* Should happen *once* per vm instantiation.
    Basically should happen in vm_init, but we're trying to keep that
    returning void (and avoiding allocation) for purity's sake. */

    #define _OBJ_VM_MKSYM(NAME, STRING) \
        vm->sym_##NAME = obj_symtable_get_sym(vm->pool->symtable, STRING); \
        if(!vm->sym_##NAME){ \
            fprintf(stderr, "%s: Couldn't get sym: %s\n", \
                __func__, STRING); \
            return 1; \
        }
    #include "vm_mksym.inc"
    #undef _OBJ_VM_MKSYM

    return 0;
}

obj_t *obj_vm_get_module(obj_vm_t *vm, obj_sym_t *name){
    return obj_dict_get(&vm->modules, name);
}

obj_t *obj_vm_get_or_add_module(obj_vm_t *vm, obj_sym_t *name){
    obj_t *module = obj_vm_get_module(vm, name);
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

obj_t *obj_module_get_def(obj_t *module, obj_sym_t *sym){
    obj_dict_t *defs = OBJ_MODULE_GET_DEFS(module);
    return obj_dict_get(defs, sym);
}

obj_t *obj_get_def(
    obj_vm_t *vm, obj_t *module, obj_dict_t *scope, obj_sym_t *sym
){
    obj_t *ref = obj_dict_get(scope, sym);
    if(ref){
        obj_sym_t *module_name = OBJ_REF_GET_MODULE_NAME(ref);
        obj_sym_t *def_name = OBJ_REF_GET_DEF_NAME(ref);
        obj_t *module = obj_vm_get_module(vm, module_name);
        if(!module)return NULL;
        return obj_module_get_def(module, def_name);
    }
    return obj_module_get_def(module, sym);
}

obj_t *obj_vm_add_def(
    obj_vm_t *vm, obj_sym_t *name,
    obj_dict_t *scope, obj_t *args, obj_t *rets, obj_t *body
){
    obj_t *def = obj_pool_add_array(vm->pool, 7);
    if(!def)return NULL;
    obj_init_sym(OBJ_ARRAY_IGET(def, 0), name);
    obj_init_dict(OBJ_ARRAY_IGET(def, 1), scope);
    obj_init_int(OBJ_ARRAY_IGET(def, 2), OBJ_LIST_LEN(args));
    obj_init_int(OBJ_ARRAY_IGET(def, 3), OBJ_LIST_LEN(rets));
    obj_init_box(OBJ_ARRAY_IGET(def, 4), args);
    obj_init_box(OBJ_ARRAY_IGET(def, 5), rets);
    obj_init_box(OBJ_ARRAY_IGET(def, 6), body);
    return def;
}

obj_frame_t *obj_vm_push_frame(obj_vm_t *vm, obj_t *module, obj_t *def){
    obj_frame_t *frame;
    if(vm->free_frame_list){
        frame = vm->free_frame_list;
        vm->free_frame_list = frame->next;
    }else{
        /* NOTE: We use calloc here, zeroing new frame's memory, because
        obj_frame_init does NOT do so.
        (See comment at top of obj_frame_init.) */
        frame = calloc(sizeof(*frame), 1);
        if(!frame)return NULL;
    }
    obj_frame_init(frame, vm->frame_list, module, def);

    obj_t *code = OBJ_DEF_GET_CODE(def);
    obj_block_t *block = obj_frame_push_block(vm, frame, code);
    if(!block){
        obj_frame_cleanup(frame);
        free(frame);
        return NULL;
    }

    vm->frame_list = frame;
    vm->n_frames++;
    return frame;
}

obj_frame_t *obj_vm_pop_frame(obj_vm_t *vm){
    obj_frame_t *frame = vm->frame_list;
    if(!frame)return NULL;
    vm->frame_list = frame->next;
    frame->next = vm->free_frame_list;
    vm->free_frame_list = frame;
    vm->n_frames--;
    return frame;
}



/********************
* obj_vm -- parsing *
********************/

int obj_vm_parse_from(
    obj_vm_t *vm, obj_dict_t *scope,
    obj_sym_t *module_name, obj_t *module,
    obj_t *body
){
    while(OBJ_TYPE(body) == OBJ_TYPE_CELL){

        /* ref_name, def_name: to be parsed, so that at the end
        we can create a reference in scope */
        obj_sym_t *def_name;
        obj_sym_t *ref_name;

        obj_t *head = OBJ_HEAD(body);
        int head_type = OBJ_TYPE(head);
        if(head_type == OBJ_TYPE_SYM){
            obj_sym_t *head_sym = OBJ_SYM(head);
            def_name = OBJ_SYM(head);
            ref_name = def_name;
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

            def_name = OBJ_SYM(def_name_obj);
            ref_name = OBJ_SYM(ref_name_obj);
        }else{
            fprintf(stderr, "%s: Expected type: SYM or CELL\n", __func__);
            return 1;
        }

        /* Check for ref name conflict in scope */
        obj_t *old_ref = obj_dict_get(scope, ref_name);
        if(old_ref){
            fprintf(stderr, "%s: Conflict: reference to ", __func__);
            obj_sym_fprint(ref_name, stderr);
            fprintf(stderr, " already in scope!\n");
            return 1;
        }

        /* Create the ref */
        obj_t *ref = obj_pool_add_array(vm->pool, 2);
        if(!ref)return 1;
        obj_init_sym(OBJ_ARRAY_IGET(ref, 0), module_name);
        obj_init_sym(OBJ_ARRAY_IGET(ref, 1), def_name);
        if(!obj_dict_set(scope, ref_name, ref))return 1;

        body = OBJ_TAIL(body);
    }
    return 0;
}


int obj_vm_parse_raw(obj_vm_t *vm,
    const char *filename, const char *text, size_t text_len
){
#   define ERRMSG() { \
        fprintf(stderr, ">>>> AT:\n"); \
        obj_dump(code, stderr, 2); \
        fprintf(stderr, "%s: ", __func__); \
    }
#   define EXPECT(OBJ, TYPE) if(OBJ_TYPE(OBJ) != OBJ_TYPE_##TYPE){ \
        ERRMSG() \
        fprintf(stderr, "Expected type: " #TYPE "\n"); \
        goto err; \
    }
#   define EXPECT_LIST(OBJ) \
    if(OBJ_TYPE(OBJ) != OBJ_TYPE_NIL && OBJ_TYPE(OBJ) != OBJ_TYPE_CELL){ \
        ERRMSG() \
        fprintf(stderr, "Expected list\n"); \
        goto err; \
    }

    int status = 1;
    obj_parser_t _parser, *parser=&_parser;
    obj_parser_init(parser, vm->pool, filename, text, text_len);
    /*** ALL CODE AFTER THIS MUST "GOTO ERR" INSTEAD OF "RETURN" ***/

    obj_t *code = obj_parser_parse(parser);
    if(!code)goto err;

    if(obj_vm_get_syms(vm))goto err;

    obj_sym_t *module_name = obj_symtable_get_sym(vm->pool->symtable, "");
    if(!module_name)goto err;
    obj_t *module = obj_vm_get_or_add_module(vm, module_name);
    if(!module)goto err;
    obj_dict_t *scope = obj_pool_dict_alloc(vm->pool);
    if(!scope)goto err;

    /* !!!TODO: BETTER ERROR LOGGING
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
        if(sym == vm->sym_module){
            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, SYM)
            module_name = OBJ_SYM(code_head);
            if(!module_name)goto err;
            module = obj_vm_get_or_add_module(vm, module_name);
            if(!module)goto err;
            scope = obj_pool_dict_alloc(vm->pool);
            if(!scope)goto err;
        }else if(sym == vm->sym_from){
            if(!module){
                ERRMSG()
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
            if(obj_vm_parse_from(
                vm, scope, module_name, module, code_head))goto err;
        }else if(sym == vm->sym_def){
            if(!module){
                ERRMSG()
                fprintf(stderr, "Illegal outside module.\n");
                goto err;
            }

            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            code_head = OBJ_HEAD(code);
            EXPECT(code_head, SYM)
            obj_sym_t *def_name = OBJ_SYM(code_head);

            /* Check for def name conflict in module */
            obj_t *old_a = obj_module_get_def(module, def_name);
            if(old_a){
                fprintf(stderr, "%s: Conflict: def ", __func__);
                obj_sym_fprint(def_name, stderr);
                fprintf(stderr, " already in module!\n");
                return 1;
            }

            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            obj_t *args = OBJ_HEAD(code);
            EXPECT_LIST(args)

            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            obj_t *rets = OBJ_HEAD(code);
            EXPECT_LIST(rets)

            code = OBJ_TAIL(code);
            EXPECT(code, CELL)
            obj_t *body = OBJ_HEAD(code);
            EXPECT_LIST(body)

            obj_dict_t *defs = OBJ_MODULE_GET_DEFS(module);
            obj_t *def = obj_vm_add_def(
                vm, def_name, scope, args, rets, body);
            if(!def)goto err;
            if(!obj_dict_set(defs, def_name, def))goto err;
        }else{
            ERRMSG()
            fprintf(stderr, "Expected one of: module, from, def, docs.\n");
            goto err;
        }
        code = OBJ_TAIL(code);
    }

    status = 0;
err:
    obj_parser_cleanup(parser);
    return status;
#   undef EXPECT
#   undef EXPECT_LIST
}


/********************
* obj_vm -- running *
********************/

int obj_vm_step(obj_vm_t *vm, bool *running_ptr){
    /* Caller is expected to set up a "bool running = true", and pass
    us &running as running_ptr.
    If we return 1, there was an error.
    If we return 0, everything's ok.
    If we set *running_ptr = false and return 0, everything's ok
    but caller should stop calling step() - that is to say, we're done
    running, presumably because we've reached the end of our program. */

#   define OBJ_STACKCHECK(N) \
    if(frame->stack_tos < (N)){ \
        fprintf(stderr, "%s: Failed stack check (%i) for: ", \
            __func__, (N)); \
        obj_sym_fprint(inst, stderr); \
        putc('\n', stderr); \
        return 1; \
    }

#   define OBJ_TYPECHECK(o, T) \
    if(OBJ_TYPE(o) != T){ \
        fprintf(stderr, "%s: Failed type check (%s) for: ", \
            __func__, obj_type_msg(T)); \
        obj_sym_fprint(inst, stderr); \
        putc('\n', stderr); \
        fprintf(stderr, "Value was:\n"); \
        obj_dump((o), stderr, 2); \
        return 1; \
    }

#   define OBJ_TYPECHECK_LIST(o) \
    if(OBJ_TYPE(o) != OBJ_TYPE_CELL && OBJ_TYPE(o) != OBJ_TYPE_NIL){ \
        fprintf(stderr, "%s: Failed type check (list) for: ", \
            __func__); \
        obj_sym_fprint(inst, stderr); \
        putc('\n', stderr); \
        fprintf(stderr, "Value was:\n"); \
        obj_dump((o), stderr, 2); \
        return 1; \
    }

#   define OBJ_FRAME_NEXT(VAR) \
        obj_t *VAR; \
        { \
            OBJ_TYPECHECK(code, OBJ_TYPE_CELL) \
            VAR = OBJ_HEAD(code); \
            code = OBJ_TAIL(code); \
        }

#   define OBJ_FRAME_NEXTSYM(VAR) \
        obj_sym_t *VAR; \
        { \
            OBJ_TYPECHECK(code, OBJ_TYPE_CELL) \
            obj_t *sym_obj = OBJ_HEAD(code); \
            code = OBJ_TAIL(code); \
            OBJ_TYPECHECK(sym_obj, OBJ_TYPE_SYM) \
            VAR = OBJ_SYM(sym_obj); \
        }

#   define OBJ_FRAME_BINOP(T) \
        OBJ_STACKCHECK(2) \
        obj_t *x = OBJ_FRAME_NOS(frame); \
        obj_t *y = OBJ_FRAME_TOS(frame); \
        OBJ_TYPECHECK(x, OBJ_TYPE_##T) \
        OBJ_TYPECHECK(y, OBJ_TYPE_##T) \
        frame->stack_tos--; \
        obj_t *z = OBJ_FRAME_TOS(frame);

    obj_frame_t *frame;
    obj_block_t *block;
    while(1){
        frame = vm->frame_list;
        if(!frame){
            *running_ptr = false;
            return 0;
        }
        block = frame->block_list;
        if(!block){
            obj_vm_pop_frame(vm);
            continue;
        }
        obj_t *code = block->code;
        if(!code || OBJ_TYPE(code) != OBJ_TYPE_CELL){
            obj_frame_pop_block(vm, frame);
            continue;
        }
        break;
    }

    obj_t *code = block->code;
    obj_t *inst_obj = OBJ_HEAD(code);
    code = OBJ_TAIL(code);

    int inst_obj_type = OBJ_TYPE(inst_obj);
    if(inst_obj_type == OBJ_TYPE_SYM){
        obj_sym_t *inst = OBJ_SYM(inst_obj);
        if(inst == vm->sym_ignore){
            OBJ_FRAME_NEXT(ignored_stuff)
        }else if(inst == vm->sym_null){
            obj_t obj;
            obj_init_null(&obj);
            if(!obj_frame_push(frame, &obj))return 1;
        }else if(inst == vm->sym_T){
            obj_t obj;
            obj_init_bool(&obj, true);
            if(!obj_frame_push(frame, &obj))return 1;
        }else if(inst == vm->sym_F){
            obj_t obj;
            obj_init_bool(&obj, false);
            if(!obj_frame_push(frame, &obj))return 1;
        }else if(inst == vm->sym_sym_lit){
            OBJ_FRAME_NEXT(sym_obj)
            OBJ_TYPECHECK(sym_obj, OBJ_TYPE_SYM)
            if(!obj_frame_push(frame, sym_obj))return 1;
        }else if(inst == vm->sym_nil){
            obj_t *obj = obj_pool_add_nil(vm->pool);
            if(!obj)return 1;
            obj_t box;
            obj_init_box(&box, obj);
            if(!obj_frame_push(frame, &box))return 1;
        }else if(inst == vm->sym_push){
            OBJ_STACKCHECK(2)
            obj_t *head = OBJ_FRAME_TOS(frame);
            obj_t *tail = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK_LIST(tail)
            obj_t *obj = obj_pool_add_cell(vm->pool, head, tail);
            if(!obj)return 1;
            obj_t box;
            obj_init_box(&box, obj);
            if(!obj_frame_push(frame, &box))return 1;
        }else if(inst == vm->sym_pop){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_init_box(OBJ_FRAME_TOS(frame), OBJ_TAIL(obj));
            if(!obj_frame_push(frame, OBJ_HEAD(obj)))return 1;
        }else if(inst == vm->sym_head){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            *OBJ_FRAME_TOS(frame) = *OBJ_HEAD(obj);
        }else if(inst == vm->sym_tail){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_init_box(OBJ_FRAME_TOS(frame), OBJ_TAIL(obj));
        }else if(inst == vm->sym_list){
            OBJ_FRAME_NEXT(obj)
            obj_t box;
            obj_init_box(&box, obj);
            if(!obj_frame_push(frame, &box))return 1;
        }else if(inst == vm->sym_list_len){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_init_int(OBJ_FRAME_TOS(frame), OBJ_LIST_LEN(obj));
        }else if(inst == vm->sym_is_null){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_FRAME_TOS(frame)) == OBJ_TYPE_NULL);
        }else if(inst == vm->sym_is_bool){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_FRAME_TOS(frame)) == OBJ_TYPE_BOOL);
        }else if(inst == vm->sym_is_int){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_FRAME_TOS(frame)) == OBJ_TYPE_INT);
        }else if(inst == vm->sym_is_sym){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_FRAME_TOS(frame)) == OBJ_TYPE_SYM);
        }else if(inst == vm->sym_is_str){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_FRAME_TOS(frame)) == OBJ_TYPE_STR);
        }else if(inst == vm->sym_is_obj){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)))
                == OBJ_TYPE_STRUCT);
        }else if(inst == vm->sym_is_dict){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_FRAME_TOS(frame)) == OBJ_TYPE_DICT);
        }else if(inst == vm->sym_is_arr){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)))
                == OBJ_TYPE_ARRAY);
        }else if(inst == vm->sym_is_nil){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)))
                == OBJ_TYPE_NIL);
        }else if(inst == vm->sym_is_list){
            OBJ_STACKCHECK(1)
            int type = OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)));
            obj_init_bool(OBJ_FRAME_TOS(frame),
                type == OBJ_TYPE_NIL || type == OBJ_TYPE_CELL);
        }else if(inst == vm->sym_not){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            OBJ_INT(OBJ_FRAME_TOS(frame)) = !OBJ_BOOL(OBJ_FRAME_TOS(frame));
        }else if(inst == vm->sym_dup){
            OBJ_STACKCHECK(1)
            if(!obj_frame_push(frame, OBJ_FRAME_TOS(frame)))return 1;
        }else if(inst == vm->sym_drop){
            OBJ_STACKCHECK(1)
            frame->stack_tos--;
        }else if(inst == vm->sym_swap){
            OBJ_STACKCHECK(2)
            obj_t tos_obj = *OBJ_FRAME_TOS(frame);
            *OBJ_FRAME_TOS(frame) = *OBJ_FRAME_NOS(frame);
            *OBJ_FRAME_NOS(frame) = tos_obj;
        }else if(inst == vm->sym_nip){
            OBJ_STACKCHECK(2)
            *OBJ_FRAME_NOS(frame) = *OBJ_FRAME_TOS(frame);
            frame->stack_tos--;
        }else if(inst == vm->sym_tuck){
            OBJ_STACKCHECK(2)
            obj_t tos_obj = *OBJ_FRAME_TOS(frame);
            *OBJ_FRAME_TOS(frame) = *OBJ_FRAME_NOS(frame);
            *OBJ_FRAME_NOS(frame) = tos_obj;
            if(!obj_frame_push(frame, &tos_obj))return 1;
        }else if(inst == vm->sym_over){
            OBJ_STACKCHECK(2)
            if(!obj_frame_push(frame, OBJ_FRAME_NOS(frame)))return 1;
        }else if(inst == vm->sym_var_get){
            OBJ_FRAME_NEXTSYM(sym)
            obj_t *var = obj_frame_get_var(frame, sym);
            if(!var){
                fprintf(stderr, "%s: Couldn't find var: ", __func__);
                obj_sym_fprint(sym, stderr);
                putc('\n', stderr);
                return 1;
            }
            if(!obj_frame_push(frame, var))return 1;
        }else if(inst == vm->sym_var_set){
            OBJ_STACKCHECK(1)
            OBJ_FRAME_NEXTSYM(sym)
            if(!obj_frame_set_var(frame, sym, OBJ_FRAME_TOS(frame)))return 1;
            frame->stack_tos--;
        }else if(inst == vm->sym_add){
            OBJ_FRAME_BINOP(INT)
            OBJ_INT(z) = OBJ_INT(x) + OBJ_INT(y);
        }else if(inst == vm->sym_sub){
            OBJ_FRAME_BINOP(INT)
            OBJ_INT(z) = OBJ_INT(x) - OBJ_INT(y);
        }else if(inst == vm->sym_mul){
            OBJ_FRAME_BINOP(INT)
            OBJ_INT(z) = OBJ_INT(x) * OBJ_INT(y);
        }else if(inst == vm->sym_div){
            OBJ_FRAME_BINOP(INT)
            OBJ_INT(z) = OBJ_INT(x) / OBJ_INT(y);
        }else if(inst == vm->sym_mod){
            OBJ_FRAME_BINOP(INT)
            OBJ_INT(z) = OBJ_INT(x) % OBJ_INT(y);
        }else if(inst == vm->sym_eq){
            OBJ_FRAME_BINOP(INT)
            obj_init_bool(z, OBJ_INT(x) == OBJ_INT(y));
        }else if(inst == vm->sym_ne){
            OBJ_FRAME_BINOP(INT)
            obj_init_bool(z, OBJ_INT(x) != OBJ_INT(y));
        }else if(inst == vm->sym_lt){
            OBJ_FRAME_BINOP(INT)
            obj_init_bool(z, OBJ_INT(x) < OBJ_INT(y));
        }else if(inst == vm->sym_le){
            OBJ_FRAME_BINOP(INT)
            obj_init_bool(z, OBJ_INT(x) <= OBJ_INT(y));
        }else if(inst == vm->sym_gt){
            OBJ_FRAME_BINOP(INT)
            obj_init_bool(z, OBJ_INT(x) > OBJ_INT(y));
        }else if(inst == vm->sym_ge){
            OBJ_FRAME_BINOP(INT)
            obj_init_bool(z, OBJ_INT(x) >= OBJ_INT(y));
        }else if(inst == vm->sym_obj){
            OBJ_FRAME_NEXT(keys)
            OBJ_TYPECHECK_LIST(keys)
            obj_t *obj = obj_pool_add_struct(vm->pool,
                OBJ_LIST_LEN(keys));
            if(!obj)return 1;
            size_t i = 0;
            while(OBJ_TYPE(keys) == OBJ_TYPE_CELL){
                obj_t *key_obj = OBJ_HEAD(keys);
                OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
                obj_sym_t *key = OBJ_SYM(key_obj);
                obj_init_sym(OBJ_STRUCT_IGET_KEY(obj, i), key);
                obj_init_null(OBJ_STRUCT_IGET_VAL(obj, i));
                keys = OBJ_TAIL(keys);
                i++;
            }
            obj_t box;
            obj_init_box(&box, obj);
            if(!obj_frame_push(frame, &box))return 1;
        }else if(inst == vm->sym_obj_get){
            OBJ_FRAME_NEXTSYM(key)
            OBJ_STACKCHECK(1)
            obj_t *s_obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(s_obj, OBJ_TYPE_STRUCT)

            obj_t *val = OBJ_STRUCT_GET(s_obj, key);
            if(!val){
                fprintf(stderr, "%s: Couldn't find struct key: ", __func__);
                obj_sym_fprint(key, stderr);
                putc('\n', stderr);
                return 1;
            }

            *OBJ_FRAME_TOS(frame) = *val;
        }else if(inst == vm->sym_obj_set){
            OBJ_FRAME_NEXTSYM(key)
            OBJ_STACKCHECK(2)
            obj_t *new_val = OBJ_FRAME_TOS(frame);
            obj_t *s_obj = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK(s_obj, OBJ_TYPE_STRUCT)

            obj_t *val = OBJ_STRUCT_GET(s_obj, key);
            if(!val){
                fprintf(stderr, "%s: Couldn't find struct key: ", __func__);
                obj_sym_fprint(key, stderr);
                putc('\n', stderr);
                return 1;
            }

            *val = *new_val;
            frame->stack_tos--;
        }else if(inst == vm->sym_dict){
            obj_t *obj = obj_pool_add_dict(vm->pool);
            if(!obj)return 1;
            if(!obj_frame_push(frame, obj))return 1;
        }else if(inst == vm->sym_dict_has){
            OBJ_STACKCHECK(2)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *d_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = OBJ_DICT_GET(d, key);
            frame->stack_tos--;
            obj_init_bool(OBJ_FRAME_TOS(frame), val != NULL);
        }else if(inst == vm->sym_dict_get){
            OBJ_STACKCHECK(2)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *d_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = OBJ_DICT_GET(d, key);
            if(!val){
                fprintf(stderr, "%s: Couldn't find dict key: ", __func__);
                obj_sym_fprint(key, stderr);
                putc('\n', stderr);
                return 1;
            }

            frame->stack_tos--;
            *OBJ_FRAME_TOS(frame) = *val;
        }else if(inst == vm->sym_dict_set){
            OBJ_STACKCHECK(3)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *new_val = OBJ_FRAME_NOS(frame);
            obj_t *d_obj = OBJ_FRAME_3OS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = OBJ_DICT_GET(d, key);
            if(!val){
                /* NOTE: dicts store obj_t*, they have no space of
                their own for actual obj_t.
                So the first time you dict_set a key, we allocate 1 obj_t. */
                val = obj_pool_objs_alloc(vm->pool, 1);
                if(!val)return 1;
            }
            *val = *new_val;

            frame->stack_tos -= 2;
            if(!obj_dict_set(d, key, val))return 1;
        }else if(inst == vm->sym_arr){
            OBJ_STACKCHECK(2)
            obj_t *val = OBJ_FRAME_TOS(frame);
            obj_t *len_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(len_obj, OBJ_TYPE_INT)
            int len = OBJ_INT(len_obj);
            if(len < 0){
                fprintf(stderr, "%s: Negative arr length: %i\n",
                    __func__, len);
                return 1;
            }
            obj_t *obj = obj_pool_add_array(vm->pool, len);
            if(!obj)return 1;
            for(size_t i = 0; i < len; i++){
                *OBJ_ARRAY_IGET(obj, i) = *val;
            }

            frame->stack_tos--;
            obj_init_box(OBJ_FRAME_TOS(frame), obj);
        }else if(inst == vm->sym_str_len){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_FRAME_TOS(frame);
            OBJ_TYPECHECK(obj, OBJ_TYPE_STR)
            obj_string_t *s = OBJ_STRING(obj);
            if((int)s->len < 0){
                /* TODO: Is this safe enough?.. */
                fprintf(stderr, "%s: String length overflow! %zu -> %i\n",
                    __func__, s->len, (int)s->len);
                return 1;
            }
            obj_init_int(OBJ_FRAME_TOS(frame), (int)s->len);
        }else if(inst == vm->sym_str_eq){
            OBJ_STACKCHECK(2)
            obj_t *s1_obj = OBJ_FRAME_NOS(frame);
            obj_t *s2_obj = OBJ_FRAME_TOS(frame);
            OBJ_TYPECHECK(s1_obj, OBJ_TYPE_STR)
            OBJ_TYPECHECK(s2_obj, OBJ_TYPE_STR)
            obj_string_t *s1 = OBJ_STRING(s1_obj);
            obj_string_t *s2 = OBJ_STRING(s2_obj);
            frame->stack_tos--;
            obj_init_bool(OBJ_FRAME_TOS(frame), obj_string_eq(s1, s2));
        }else if(inst == vm->sym_arr_len){
            OBJ_STACKCHECK(1)
            obj_t *a_obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(a_obj, OBJ_TYPE_ARRAY)
            obj_init_int(OBJ_FRAME_TOS(frame), OBJ_ARRAY_LEN(a_obj));
        }else if(inst == vm->sym_arr_iget){
            OBJ_STACKCHECK(2)

            obj_t *i_obj = OBJ_FRAME_TOS(frame);
            obj_t *a_obj = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(a_obj, OBJ_TYPE_ARRAY)
            int i = OBJ_INT(i_obj);
            int len = OBJ_ARRAY_LEN(a_obj);

            if(i < 0 || i >= len){
                fprintf(stderr,
                    "%s: Array index %i out of range for len: %i\n",
                    __func__, i, len);
                return 1;
            }
            obj_t *val = OBJ_ARRAY_IGET(a_obj, i);

            frame->stack_tos--;
            *OBJ_FRAME_TOS(frame) = *val;
        }else if(inst == vm->sym_arr_iset){
            OBJ_STACKCHECK(3)

            obj_t *i_obj = OBJ_FRAME_TOS(frame);
            obj_t *val = OBJ_FRAME_NOS(frame);
            obj_t *a_obj = OBJ_RESOLVE(OBJ_FRAME_3OS(frame));
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(a_obj, OBJ_TYPE_ARRAY)
            int i = OBJ_INT(i_obj);
            int len = OBJ_ARRAY_LEN(a_obj);

            if(i < 0 || i >= len){
                fprintf(stderr,
                    "%s: Array index %i out of range for len: %i\n",
                    __func__, i, len);
                return 1;
            }
            *OBJ_ARRAY_IGET(a_obj, i) = *val;

            frame->stack_tos -= 2;
        }else if(inst == vm->sym_call){
            OBJ_FRAME_NEXTSYM(sym)
            obj_t *module = frame->module;
            obj_dict_t *scope = OBJ_DEF_GET_SCOPE(frame->def);
            obj_t *def = obj_get_def(vm, module, scope, sym);
            if(!def){
                obj_sym_t *module_name = OBJ_MODULE_GET_NAME(module);
                fprintf(stderr, "%s: Couldn't find @@ ", __func__);
                obj_sym_fprint(module_name, stderr);
                putc(' ', stderr);
                obj_sym_fprint(sym, stderr);
                putc('\n', stderr);
                return 1;
            }
            if(!obj_vm_push_frame(vm, module, def))return 1;
        }else if(inst == vm->sym_longcall){
            OBJ_FRAME_NEXTSYM(module_name)
            OBJ_FRAME_NEXTSYM(sym)
            obj_t *module = obj_vm_get_module(vm, module_name);
            if(!module){
                fprintf(stderr, "%s: Couldn't find module: ", __func__);
                obj_sym_fprint(module_name, stderr);
                putc('\n', stderr);
                return 1;
            }
            obj_t *def = obj_module_get_def(module, sym);
            if(!def){
                fprintf(stderr, "%s: Couldn't find @@ ", __func__);
                obj_sym_fprint(module_name, stderr);
                putc(' ', stderr);
                obj_sym_fprint(sym, stderr);
                putc('\n', stderr);
                return 1;
            }
            if(!obj_vm_push_frame(vm, module, def))return 1;
        }else if(inst == vm->sym_p){
            OBJ_STACKCHECK(1)
            obj_dump(OBJ_FRAME_TOS(frame), stderr, 0);
            frame->stack_tos--;
        }else if(inst == vm->sym_str_p){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_STR)
            obj_string_t *s = OBJ_STRING(OBJ_FRAME_TOS(frame));
            fprintf(stderr, "%.*s", (int)s->len, s->data);
            frame->stack_tos--;
        }else if(inst == vm->sym_assert){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            if(!OBJ_BOOL(OBJ_FRAME_TOS(frame))){
                fprintf(stderr, "%s: Failed assertion!\n", __func__);
                return 1;
            }
            frame->stack_tos--;
        }else if(inst == vm->sym_p_stack)obj_frame_dump_stack(frame, stderr, 0);
        else if(inst == vm->sym_p_vars)obj_frame_dump_vars(frame, stderr, 0);
        else if(inst == vm->sym_p_blocks)obj_frame_dump_blocks(frame, stderr, 0);
        else if(inst == vm->sym_p_frame)obj_frame_dump(frame, stderr, 0);
        else{
            fprintf(stderr, "INST: ");
            obj_sym_fprint(inst, stderr);
            putc('\n', stderr);
        }
    }else if(
        inst_obj_type == OBJ_TYPE_CELL ||
        inst_obj_type == OBJ_TYPE_NIL
    ){
        if(!obj_frame_push_block(vm, frame, inst_obj))return 1;
    }else{
        if(!obj_frame_push(frame, inst_obj))return 1;
    }
    block->code = code;
    return 0;
#   undef OBJ_STACKCHECK
#   undef OBJ_TYPECHECK
#   undef OBJ_TYPECHECK_LIST
#   undef OBJ_FRAME_NEXT
#   undef OBJ_FRAME_NEXTSYM
#   undef OBJ_FRAME_BINOP
}

int obj_vm_run(obj_vm_t *vm){
    bool running = true;
    while(running){
        if(obj_vm_step(vm, &running))goto err;
    }
    return 0;
err:
    fprintf(stderr,
        "%s: Error while executing! Frame dump:\n", __func__);
    obj_vm_dump_frames(vm, stderr, 0);
    fprintf(stderr,
        "%s: Error while executing! See frame dump above.\n", __func__);
    return 1;
}


#endif