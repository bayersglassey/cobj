#ifndef _COBJ_LANG_H_
#define _COBJ_LANG_H_

#include "cobj.h"
#include "utils.h"


#define OBJ_FRAME_DEFAULT_VARS_LEN 8
#define OBJ_FRAME_DEFAULT_STACK_LEN 8

/* TOS: Top Of Stack, NOS: Next On Stack, 3OS: Third On Stack */
#define OBJ_FRAME_GET(frame, i) &frame->stack[frame->stack_tos - (i) - 1]
#define OBJ_FRAME_TOS(frame) OBJ_FRAME_GET(frame, 0)
#define OBJ_FRAME_NOS(frame) OBJ_FRAME_GET(frame, 1)
#define OBJ_FRAME_3OS(frame) OBJ_FRAME_GET(frame, 2)

#define OBJ_MODULE_NAME(module) OBJ_SYM(OBJ_ARRAY_IGET(module, 0))
#define OBJ_MODULE_DEFS(module) OBJ_DICT(OBJ_ARRAY_IGET(module, 1))

#define OBJ_DEF_MODULE(vm, def) obj_vm_get_module(vm, \
    OBJ_DEF_MODULE_NAME(def))
#define OBJ_DEF_MODULE_NAME(def) OBJ_SYM(OBJ_ARRAY_IGET(def, 0))
#define OBJ_DEF_NAME(def) OBJ_SYM(OBJ_ARRAY_IGET(def, 1))
#define OBJ_DEF_SCOPE(def) OBJ_DICT(OBJ_ARRAY_IGET(def, 2))
#define OBJ_DEF_N_ARGS(def) OBJ_INT(OBJ_ARRAY_IGET(def, 3))
#define OBJ_DEF_N_RETS(def) OBJ_INT(OBJ_ARRAY_IGET(def, 4))
#define OBJ_DEF_ARGS(def) OBJ_CONTENTS(OBJ_ARRAY_IGET(def, 5))
#define OBJ_DEF_RETS(def) OBJ_CONTENTS(OBJ_ARRAY_IGET(def, 6))
#define OBJ_DEF_CODE(def) OBJ_CONTENTS(OBJ_ARRAY_IGET(def, 7))

#define OBJ_REF_MODULE_NAME(ref) OBJ_SYM(OBJ_ARRAY_IGET(ref, 0))
#define OBJ_REF_DEF_NAME(ref) OBJ_SYM(OBJ_ARRAY_IGET(ref, 1))


typedef struct obj_vm obj_vm_t;
typedef struct obj_frame obj_frame_t;
typedef struct obj_block obj_block_t;

enum {
    OBJ_BLOCK_BASIC,
    OBJ_BLOCK_DO,
    OBJ_BLOCK_FOR,
    OBJ_BLOCK_INT_FOR,
    OBJ_BLOCK_LIST_FOR,
    OBJ_BLOCKS
};


struct obj_block {
    obj_block_t *next;

    int type;
    union {
        obj_t *o;
        struct {
            int i;
            int n;
        } int_for;
    } u;

    obj_t *code_start;
    obj_t *code;
        /* code_start, code: OBJ_TYPE_CELL or OBJ_TYPE_NIL */
        /* code_start and code are the same when block is initialized.
        Then, with each step of execution, code = OBJ_TAIL(code).
        But code_start doesn't change, so e.g. if block->type
        == OBJ_BLOCK_FOR, then when we reach the end of block->code
        we can set block->code = block->code_start to continue the
        loop. */
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

void obj_block_init(
    obj_block_t *block, obj_block_t *next, int type, obj_t *code
){
    memset(block, 0, sizeof(*block));
    block->next = next;
    block->type = type;
    block->code_start = code;
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

void obj_frame_dump(obj_frame_t *frame, FILE *file, int depth, bool dump_def){
    _print_tabs(file, depth);
    fprintf(file, "FRAME %p:\n", frame);
    _print_tabs(file, depth);
    fprintf(file, "  DEF: ");
    if(dump_def){
        obj_dump(frame->def, file, depth + 4);
        putc('\n', file);
    }else{
        obj_sym_fprint(OBJ_DEF_MODULE_NAME(frame->def), file);
        putc(' ', file);
        obj_sym_fprint(OBJ_DEF_NAME(frame->def), file);
        putc('\n', file);
    }
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
    obj_vm_t *vm, obj_frame_t *frame, int block_type, obj_t *code
){
    obj_block_t *block;
    if(vm->free_block_list){
        block = vm->free_block_list;
        vm->free_block_list = block->next;
    }else{
        block = calloc(sizeof(*block), 1);
        if(!block)return NULL;
    }
    obj_block_init(block, frame->block_list, block_type, code);
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

void obj_vm_dump_frames(
    obj_vm_t *vm, FILE *file, int depth, bool dump_frame_defs
){
    _print_tabs(file, depth);
    fprintf(file, "FRAMES (%i):\n", vm->n_frames);
    for(obj_frame_t *frame = vm->frame_list;
        frame; frame = frame->next
    ){
        obj_frame_dump(frame, file, depth + 2, dump_frame_defs);
    }
}

void obj_vm_dump(obj_vm_t *vm, FILE *file){
    fprintf(file, "VM %p:\n", vm);
    obj_vm_dump_modules(vm, file, 2);
    obj_vm_dump_frames(vm, file, 2, true);
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
    obj_dict_t *defs = OBJ_MODULE_DEFS(module);
    return obj_dict_get(defs, sym);
}

obj_t *obj_get_def(
    obj_vm_t *vm, obj_t *module, obj_dict_t *scope, obj_sym_t *sym
){
    obj_t *ref = obj_dict_get(scope, sym);
    if(ref){
        obj_sym_t *module_name = OBJ_REF_MODULE_NAME(ref);
        obj_sym_t *def_name = OBJ_REF_DEF_NAME(ref);
        obj_t *module = obj_vm_get_module(vm, module_name);
        if(!module)return NULL;
        return obj_module_get_def(module, def_name);
    }
    return obj_module_get_def(module, sym);
}

obj_t *obj_vm_add_def(
    obj_vm_t *vm, obj_sym_t *module_name, obj_sym_t *name,
    obj_dict_t *scope, obj_t *args, obj_t *rets, obj_t *body
){
    obj_t *def = obj_pool_add_array(vm->pool, 8);
    if(!def)return NULL;
    obj_init_sym(OBJ_ARRAY_IGET(def, 0), module_name);
    obj_init_sym(OBJ_ARRAY_IGET(def, 1), name);
    obj_init_dict(OBJ_ARRAY_IGET(def, 2), scope);
    obj_init_int(OBJ_ARRAY_IGET(def, 3), OBJ_LIST_LEN(args));
    obj_init_int(OBJ_ARRAY_IGET(def, 4), OBJ_LIST_LEN(rets));
    obj_init_box(OBJ_ARRAY_IGET(def, 5), args);
    obj_init_box(OBJ_ARRAY_IGET(def, 6), rets);
    obj_init_box(OBJ_ARRAY_IGET(def, 7), body);
    return def;
}

obj_frame_t *obj_vm_push_frame(obj_vm_t *vm, obj_t *module, obj_t *def){
    obj_frame_t *parent_frame = vm->frame_list;

    /* Check that parent_frame->stack has enough values for def's args */
    int n_args = OBJ_DEF_N_ARGS(def);
    size_t parent_frame_stack_tos = parent_frame?
        parent_frame->stack_tos: 0;
    if(parent_frame_stack_tos < n_args){
        fprintf(stderr, "%s: stack too small for call: %zu < %i\n",
            __func__, parent_frame_stack_tos, n_args);
        return NULL;
    }

    /* Create frame (or get it from the free list) */
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

    /* Initialize frame and push it onto vm */
    obj_frame_init(frame, vm->frame_list, module, def);
    vm->frame_list = frame;
    vm->n_frames++;

    /* Move n_args values from parent_frame->stack to frame->stack */
    for(int i = n_args - 1; i >= 0; i--){
        obj_t *arg = &parent_frame->stack[parent_frame->stack_tos - i - 1];
        if(!obj_frame_push(frame, arg))return NULL;
    }
    if(parent_frame)parent_frame->stack_tos -= n_args;

    /* Push def's code as a block onto frame */
    obj_t *code = OBJ_DEF_CODE(def);
    obj_block_t *block = obj_frame_push_block(
        vm, frame, OBJ_BLOCK_BASIC, code);
    if(!block)return NULL;

    return frame;
}

obj_frame_t *obj_vm_pop_frame(obj_vm_t *vm){
    obj_frame_t *frame = vm->frame_list;
    if(!frame)return NULL;

    /* Check that frame->stack has correct number of values for
    def's rets */
    int n_rets = OBJ_DEF_N_RETS(frame->def);
    if(frame->stack_tos != n_rets){
        fprintf(stderr, "%s: stack wrong size for return: %zu != %i\n",
            __func__, frame->stack_tos, n_rets);
        return NULL;
    }

    /* Check that parent_frame exists if we're attempting to return
    values to it */
    obj_frame_t *parent_frame = frame->next;
    if(n_rets > 0 && !parent_frame){
        fprintf(stderr,
            "%s: can't return arguments without a parent frame!\n",
            __func__);
        return NULL;
    }

    /* Move n_rets values from frame->stack to parent_frame->stack */
    for(int i = n_rets - 1; i >= 0; i--){
        obj_t *ret = &frame->stack[frame->stack_tos - i - 1];
        if(!obj_frame_push(parent_frame, ret))return NULL;
    }

    /* Pop frame from vm */
    vm->frame_list = parent_frame;
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
            if(head_len != 2){
                fprintf(stderr, "%s: Expected list of length 2, got %i\n",
                    __func__, head_len);
                return 1;
            }
            obj_t *def_name_obj = OBJ_HEAD(head);
            head = OBJ_TAIL(head);
            obj_t *ref_name_obj = OBJ_HEAD(head);
            if(OBJ_TYPE(def_name_obj) != OBJ_TYPE_SYM){
                fprintf(stderr, "%s: Expected def_name type: SYM\n",
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
        fprintf(stderr, "Expected type: %s\n", #TYPE); \
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

            obj_sym_t *module_name = OBJ_MODULE_NAME(module);
            obj_dict_t *defs = OBJ_MODULE_DEFS(module);
            obj_t *def = obj_vm_add_def(
                vm, module_name, def_name, scope, args, rets, body);
            if(!def)goto err;
            if(!obj_dict_set(defs, def_name, def))goto err;
        }else{
            ERRMSG()
            fprintf(stderr, "Expected one of: module, from, def.\n");
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

int obj_vm_step(obj_vm_t *vm, bool *running_ptr);
#include "lang_step.h" /* definition of obj_vm_step */

int obj_vm_run(obj_vm_t *vm){
    bool running = true;
    while(running){
        if(obj_vm_step(vm, &running))goto err;
    }
    return 0;
err:
    fprintf(stderr,
        "%s: Error while executing! Frame dump:\n", __func__);
    obj_vm_dump_frames(vm, stderr, 0, false);
    fprintf(stderr,
        "%s: Error while executing! See frame dump above.\n", __func__);
    return 1;
}


#endif