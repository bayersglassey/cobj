
/* NOTE: Expected to be #included by lang.h */


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
            if(!obj_vm_pop_frame(vm))return 1;
            continue;
        }
        int block_type = block->type;
        obj_t *code = block->code;
        if(!code || OBJ_TYPE(code) != OBJ_TYPE_CELL){
            /* End of block's code */
            if(block_type == OBJ_BLOCK_FOR){
                if(!obj_frame_push_block(
                    vm, frame, OBJ_BLOCK_BASIC, block->u.o))return 1;
                block->code = block->code_start;
                continue;
            }else if(block_type == OBJ_BLOCK_INT_FOR){
                block->u.int_for.i++;
                block->code = block->code_start;
                continue;
            }else if(block_type == OBJ_BLOCK_LIST_FOR){

                /* I'm not sure if it's ever possible for the following
                check to fail. Better safe than sorry. */
                if(OBJ_TYPE(block->u.o) == OBJ_TYPE_CELL){
                    block->u.o = OBJ_TAIL(block->u.o);
                }

                block->code = block->code_start;
                continue;
            }else{
                if(!obj_frame_pop_block(vm, frame))return 1;
                continue;
            }
        }else if(block->code == block->code_start){
            /* Start of block's code */
            if(block_type == OBJ_BLOCK_INT_FOR){
                if(block->u.int_for.i >= block->u.int_for.n){
                    if(!obj_frame_pop_block(vm, frame))return 1;
                    continue;
                }
                obj_t i_obj;
                obj_init_int(&i_obj, block->u.int_for.i);
                if(!obj_frame_push(frame, &i_obj))return 1;
            }else if(block_type == OBJ_BLOCK_LIST_FOR){
                if(OBJ_TYPE(block->u.o) != OBJ_TYPE_CELL){
                    if(!obj_frame_pop_block(vm, frame))return 1;
                    continue;
                }
                if(!obj_frame_push(frame,
                    OBJ_HEAD(block->u.o)))return 1;
            }
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
            obj_t *tail = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK_LIST(tail)

            /* NOTE: we can't just use TOS as the head!
            Because obj_t on the stack are ephemeral.
            Gotta allocate a copy of it... */
            obj_t *head = obj_pool_objs_alloc(vm->pool, 1);
            if(!head)return 1;
            *head = *OBJ_FRAME_TOS(frame);

            obj_t *obj = obj_pool_add_cell(vm->pool, head, tail);
            if(!obj)return 1;
            frame->stack_tos--;
            obj_init_box(OBJ_FRAME_TOS(frame), obj);
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
        }else if(inst == vm->sym_rev){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_t *rev_obj = obj_pool_add_rev_list(vm->pool, obj);
            if(!rev_obj)return 1;
            obj_init_box(OBJ_FRAME_TOS(frame), rev_obj);
        }else if(inst == vm->sym_flat){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_t *a_obj = obj_pool_add_array_from_list(vm->pool, obj);
            if(!a_obj)return 1;
            obj_init_box(OBJ_FRAME_TOS(frame), a_obj);
        }else if(inst == vm->sym_rev_flat){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_t *a_obj = obj_pool_add_array_from_rev_list(vm->pool, obj);
            if(!a_obj)return 1;
            obj_init_box(OBJ_FRAME_TOS(frame), a_obj);
        }else if(inst == vm->sym_queue){
            obj_t *obj = obj_pool_add_queue(vm->pool, &vm->pool->nil);
            obj_t box;
            obj_init_box(&box, obj);
            if(!obj_frame_push(frame, &box))return 1;
        }else if(inst == vm->sym_queue_push){
            OBJ_STACKCHECK(2)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK(obj, OBJ_TYPE_QUEUE)

            /* NOTE: we can't just use TOS as the head!
            Because obj_t on the stack are ephemeral.
            Gotta allocate a copy of it... */
            obj_t *head = obj_pool_objs_alloc(vm->pool, 1);
            if(!head)return 1;
            *head = *OBJ_FRAME_TOS(frame);

            obj_t *cell = obj_pool_add_cell(vm->pool,
                head, &vm->pool->nil);
            if(!cell)return 1;

            *OBJ_QUEUE_END(obj) = cell;
            OBJ_QUEUE_END(obj) = &OBJ_TAIL(cell);
            frame->stack_tos--;
        }else if(inst == vm->sym_queue_tolist){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(obj, OBJ_TYPE_QUEUE)
            obj_init_box(OBJ_FRAME_TOS(frame), OBJ_QUEUE_LIST(obj));
        }else if(inst == vm->sym_list_toqueue){
            OBJ_STACKCHECK(1)
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(obj)
            obj_t *q_obj = obj_pool_add_queue(vm->pool, obj);
            obj_init_box(OBJ_FRAME_TOS(frame), q_obj);
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
        }else if(inst == vm->sym_is_cell){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)))
                == OBJ_TYPE_CELL);
        }else if(inst == vm->sym_is_list){
            OBJ_STACKCHECK(1)
            int type = OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)));
            obj_init_bool(OBJ_FRAME_TOS(frame),
                type == OBJ_TYPE_NIL || type == OBJ_TYPE_CELL);
        }else if(inst == vm->sym_is_queue){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)))
                == OBJ_TYPE_QUEUE);
        }else if(inst == vm->sym_is_fun){
            OBJ_STACKCHECK(1)
            obj_init_bool(OBJ_FRAME_TOS(frame),
                OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)))
                == OBJ_TYPE_FUN);
        }else if(inst == vm->sym_not){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            OBJ_INT(OBJ_FRAME_TOS(frame)) = !OBJ_BOOL(OBJ_FRAME_TOS(frame));
        }else if(inst == vm->sym_bool_eq){
            OBJ_FRAME_BINOP(BOOL)
            obj_init_bool(z, OBJ_BOOL(x) == OBJ_BOOL(y));
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
        }else if(inst == vm->sym_typeof){
            OBJ_STACKCHECK(1)
            int type = OBJ_TYPE(OBJ_RESOLVE(OBJ_FRAME_TOS(frame)));
            obj_sym_t *sym =
                type == OBJ_TYPE_NULL? vm->sym_null
                : type == OBJ_TYPE_BOOL? vm->sym_bool
                : type == OBJ_TYPE_INT? vm->sym_int
                : type == OBJ_TYPE_SYM? vm->sym_sym
                : type == OBJ_TYPE_STR? vm->sym_str
                : type == OBJ_TYPE_NIL || type == OBJ_TYPE_CELL?
                    vm->sym_list
                : type == OBJ_TYPE_QUEUE? vm->sym_queue
                : type == OBJ_TYPE_ARRAY? vm->sym_arr
                : type == OBJ_TYPE_DICT? vm->sym_dict
                : type == OBJ_TYPE_STRUCT? vm->sym_obj
                : type == OBJ_TYPE_FUN? vm->sym_fun
                : NULL;
            if(sym == NULL){
                fprintf(stderr, "%s: Unrecognized type: %i (%s)\n",
                    __func__, type, obj_type_msg(type));
                return 1;
            }
            obj_init_sym(OBJ_FRAME_TOS(frame), sym);
        }else if(inst == vm->sym_sym_eq){
            OBJ_FRAME_BINOP(SYM)
            obj_init_bool(z, OBJ_SYM(x) == OBJ_SYM(y));
        }else if(inst == vm->sym_sym_tostr){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_SYM)
            obj_sym_t *sym = OBJ_SYM(OBJ_FRAME_TOS(frame));

            /* Don't let people mess with the actual string for the sym!..
            Then they could change the sym! */
            obj_string_t *s_clone = obj_pool_string_add_raw(
                vm->pool, sym->string.data, sym->string.len);
            if(!s_clone)return 1;

            obj_init_str(OBJ_FRAME_TOS(frame), s_clone);
        }else if(inst == vm->sym_str_tosym){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_STR)
            obj_string_t *s = OBJ_STRING(OBJ_FRAME_TOS(frame));
            obj_sym_t *sym = obj_symtable_get_sym_raw(
                vm->pool->symtable, s->data, s->len);
            if(!sym)return 1;
            obj_init_sym(OBJ_FRAME_TOS(frame), sym);
        }else if(inst == vm->sym_str_clone){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_STR)
            obj_string_t *s = OBJ_STRING(OBJ_FRAME_TOS(frame));
            obj_string_t *s_clone = obj_pool_string_add_raw(
                vm->pool, s->data, s->len);
            if(!s_clone)return 1;
            obj_init_str(OBJ_FRAME_TOS(frame), s_clone);
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
        }else if(inst == vm->sym_int_tostr){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_INT)
            int i = OBJ_INT(OBJ_FRAME_TOS(frame));
            size_t len = strlen_of_int(i);
            obj_string_t *s = obj_pool_string_alloc(vm->pool, len);
            if(!s)return 1;
            strncpy_of_int(s->data, i, len);
            obj_init_str(OBJ_FRAME_TOS(frame), s);
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
        }else if(inst == vm->sym_obj_len){
            OBJ_STACKCHECK(1)
            obj_t *s_obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(s_obj, OBJ_TYPE_STRUCT)
            obj_init_int(OBJ_FRAME_TOS(frame), OBJ_STRUCT_LEN(s_obj));
        }else if(inst == vm->sym_obj_iget_key){
            OBJ_STACKCHECK(2)
            obj_t *i_obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(obj, OBJ_TYPE_STRUCT)

            int i = OBJ_INT(i_obj);
            int len = OBJ_STRUCT_LEN(obj);
            if(i < 0 || i >= len){
                fprintf(stderr,
                    "%s: Obj index %i out of range for len: %i\n",
                    __func__, i, len);
                return 1;
            }

            frame->stack_tos--;
            *OBJ_FRAME_TOS(frame) = *OBJ_STRUCT_IGET_KEY(obj, i);
        }else if(inst == vm->sym_obj_iget_val){
            OBJ_STACKCHECK(2)
            obj_t *i_obj = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            obj_t *obj = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(obj, OBJ_TYPE_STRUCT)

            int i = OBJ_INT(i_obj);
            int len = OBJ_STRUCT_LEN(obj);
            if(i < 0 || i >= len){
                fprintf(stderr,
                    "%s: Obj index %i out of range for len: %i\n",
                    __func__, i, len);
                return 1;
            }

            frame->stack_tos--;
            *OBJ_FRAME_TOS(frame) = *OBJ_STRUCT_IGET_VAL(obj, i);
        }else if(inst == vm->sym_dict){
            obj_t *obj = obj_pool_add_dict(vm->pool);
            if(!obj)return 1;
            if(!obj_frame_push(frame, obj))return 1;
        }else if(inst == vm->sym_has){
            OBJ_STACKCHECK(2)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *d_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = obj_dict_get(d, key);
            frame->stack_tos--;
            obj_init_bool(OBJ_FRAME_TOS(frame), val != NULL);
        }else if(inst == vm->sym_get){
            OBJ_STACKCHECK(2)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *d_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = obj_dict_get(d, key);
            if(!val){
                fprintf(stderr, "%s: Couldn't find dict key: ", __func__);
                obj_sym_fprint(key, stderr);
                putc('\n', stderr);
                return 1;
            }

            frame->stack_tos--;
            *OBJ_FRAME_TOS(frame) = *val;
        }else if(inst == vm->sym_set){
            OBJ_STACKCHECK(3)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *new_val = OBJ_FRAME_NOS(frame);
            obj_t *d_obj = OBJ_FRAME_3OS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = obj_dict_get(d, key);
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
        }else if(inst == vm->sym_del){
            OBJ_STACKCHECK(2)

            obj_t *key_obj = OBJ_FRAME_TOS(frame);
            obj_t *d_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(key_obj, OBJ_TYPE_SYM)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_sym_t *key = OBJ_SYM(key_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);

            obj_t *val = obj_dict_del(d, key);
            if(!val){
                fprintf(stderr, "%s: Couldn't find dict key: ", __func__);
                obj_sym_fprint(key, stderr);
                putc('\n', stderr);
                return 1;
            }

            frame->stack_tos--;
        }else if(inst == vm->sym_dict_len){
            OBJ_STACKCHECK(1)
            obj_t *d_obj = OBJ_FRAME_TOS(frame);
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_init_int(OBJ_FRAME_TOS(frame), OBJ_DICT_LEN(d_obj));
        }else if(inst == vm->sym_dict_n_keys){
            OBJ_STACKCHECK(1)
            obj_t *d_obj = OBJ_FRAME_TOS(frame);
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            obj_init_int(OBJ_FRAME_TOS(frame), OBJ_DICT_N_KEYS(d_obj));
        }else if(
            inst == vm->sym_dict_ihas ||
            inst == vm->sym_dict_iget_key ||
            inst == vm->sym_dict_iget_val
        ){
            OBJ_STACKCHECK(2)

            obj_t *i_obj = OBJ_FRAME_TOS(frame);
            obj_t *d_obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(d_obj, OBJ_TYPE_DICT)
            int i = OBJ_INT(i_obj);
            obj_dict_t *d = OBJ_DICT(d_obj);
            int len = d->entries_len;

            if(i < 0 || i >= len){
                fprintf(stderr,
                    "%s: Dict index %i out of range for len: %i\n",
                    __func__, i, len);
                return 1;
            }


            frame->stack_tos--;
            if(inst == vm->sym_dict_ihas){
                obj_init_bool(OBJ_FRAME_TOS(frame), d->entries[i].sym);
            }else if(inst == vm->sym_dict_iget_key){
                obj_init_sym(OBJ_FRAME_TOS(frame), d->entries[i].sym);
            }else{
                *OBJ_FRAME_TOS(frame) = *(obj_t*)d->entries[i].value;
            }
        }else if(inst == vm->sym_arr){
            OBJ_STACKCHECK(2)
            obj_t *len_obj = OBJ_FRAME_TOS(frame);
            obj_t *val = OBJ_FRAME_NOS(frame);
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
                /* TODO: Is this safe enough?.. shouldn't we do this
                somewhere else so we never end up with a string this
                long?.. */
                fprintf(stderr, "%s: String length overflow! %zu -> %i\n",
                    __func__, s->len, (int)s->len);
                return 1;
            }
            obj_init_int(OBJ_FRAME_TOS(frame), (int)s->len);
        }else if(inst == vm->sym_str_getbyte){
            OBJ_STACKCHECK(2)
            obj_t *i_obj = OBJ_FRAME_TOS(frame);
            obj_t *obj = OBJ_FRAME_NOS(frame);
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(obj, OBJ_TYPE_STR)
            int i = OBJ_INT(i_obj);
            obj_string_t *s = OBJ_STRING(obj);

            if(i < 0 || i >= s->len){
                fprintf(stderr,
                    "%s: String index %i out of range for len: %zu\n",
                    __func__, i, s->len);
                return 1;
            }

            frame->stack_tos--;
            obj_init_int(OBJ_FRAME_TOS(frame), s->data[i]);
        }else if(inst == vm->sym_str_setbyte){
            OBJ_STACKCHECK(3)
            obj_t *i_obj = OBJ_FRAME_TOS(frame);
            obj_t *byte_obj = OBJ_FRAME_NOS(frame);
            obj_t *obj = OBJ_FRAME_3OS(frame);
            OBJ_TYPECHECK(i_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(byte_obj, OBJ_TYPE_INT)
            OBJ_TYPECHECK(obj, OBJ_TYPE_STR)
            int i = OBJ_INT(i_obj);
            int byte = OBJ_INT(byte_obj);
            obj_string_t *s = OBJ_STRING(obj);

            if(byte < 0 || byte >= 256){
                fprintf(stderr,
                    "%s: Not a byte: %i\n", __func__, byte);
                return 1;
            }
            if(i < 0 || i >= s->len){
                fprintf(stderr,
                    "%s: String index %i out of range for len: %zu\n",
                    __func__, i, s->len);
                return 1;
            }

            s->data[i] = byte;
            frame->stack_tos -= 2;
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
        }else if(inst == vm->sym_str_join){
            OBJ_STACKCHECK(2)
            obj_t *s1_obj = OBJ_FRAME_NOS(frame);
            obj_t *s2_obj = OBJ_FRAME_TOS(frame);
            OBJ_TYPECHECK(s1_obj, OBJ_TYPE_STR)
            OBJ_TYPECHECK(s2_obj, OBJ_TYPE_STR)
            obj_string_t *s1 = OBJ_STRING(s1_obj);
            obj_string_t *s2 = OBJ_STRING(s2_obj);

            obj_string_t *s3 = obj_pool_string_alloc(vm->pool,
                s1->len + s2->len);
            if(!s3)return 1;
            memcpy(s3->data, s1->data, s1->len);
            memcpy(s3->data + s1->len, s2->data, s2->len);

            frame->stack_tos--;
            obj_init_str(OBJ_FRAME_TOS(frame), s3);
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
        }else if(
            inst == vm->sym_call ||
            inst == vm->sym_ref
        ){
            OBJ_FRAME_NEXTSYM(sym)
            obj_t *module = frame->module;
            obj_dict_t *scope = OBJ_DEF_SCOPE(frame->def);
            obj_t *def = obj_get_def(vm, module, scope, sym);
            if(!def){
                fprintf(stderr, "%s: Couldn't find def: ", __func__);
                obj_sym_fprint(sym, stderr);
                putc('\n', stderr);
                fprintf(stderr, "...module was: ");
                obj_sym_fprint(OBJ_MODULE_NAME(module), stderr);
                putc('\n', stderr);
                fprintf(stderr, "...scope contained: ");
                obj_dict_fprint(scope, stderr, 2);
                putc('\n', stderr);
                return 1;
            }
            obj_t *def_module = OBJ_DEF_MODULE(vm, def);
            if(inst == vm->sym_call){
                if(!obj_vm_push_frame(vm, def_module, def))return 1;
            }else{
                obj_sym_t *module_name = OBJ_DEF_MODULE_NAME(def);
                obj_t *args = obj_pool_add_nil(vm->pool);
                if(!args)return 1;
                obj_t *obj = obj_pool_add_fun(vm->pool,
                    module_name, sym, args);
                if(!obj)return 1;
                obj_t box;
                obj_init_box(&box, obj);
                if(!obj_frame_push(frame, &box))return 1;
            }
        }else if(
            inst == vm->sym_longcall ||
            inst == vm->sym_fun_call
        ){

            /* PREPARE THYSELF FOR GOTO ABUSE
            ...because we'd like to use if/else like regular people,
            but OBJ_FRAME_NEXTSYM declares a variable, which would get
            scoped within the if/else, which we don't want */
            if(inst == vm->sym_fun_call)goto fun_call;
            OBJ_FRAME_NEXTSYM(module_name)
            OBJ_FRAME_NEXTSYM(sym)
            goto longcall;
fun_call:
            OBJ_STACKCHECK(1)
            obj_t *fun = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(fun, OBJ_TYPE_FUN)
            module_name = OBJ_FUN_MODULE_NAME(fun);
            sym = OBJ_FUN_DEF_NAME(fun);
            frame->stack_tos--;
            {
                /* Push fun's args onto stack before the call.
                We could get crazy with like, obj_frame_push_many,
                but... this is fine. It's fine. */
                obj_t *args = OBJ_FUN_ARGS(fun);
                while(OBJ_TYPE(args) == OBJ_TYPE_CELL){
                    obj_t *arg = OBJ_HEAD(args);
                    if(!obj_frame_push(frame, arg))return 1;
                    args = OBJ_TAIL(args);
                }
            }
longcall:
            ;

            obj_t *module = obj_vm_get_module(vm, module_name);
            if(!module){
                fprintf(stderr, "%s: Couldn't find module: ", __func__);
                obj_sym_fprint(module_name, stderr);
                putc('\n', stderr);
                return 1;
            }
            obj_t *def = obj_module_get_def(module, sym);
            if(!def){
                fprintf(stderr, "%s: Couldn't find def: ", __func__);
                obj_sym_fprint(module_name, stderr);
                putc(' ', stderr);
                obj_sym_fprint(sym, stderr);
                putc('\n', stderr);
                return 1;
            }
            if(!obj_vm_push_frame(vm, module, def))return 1;
        }else if(inst == vm->sym_longref){
            OBJ_FRAME_NEXTSYM(module_name)
            OBJ_FRAME_NEXTSYM(sym)
            obj_t *args = obj_pool_add_nil(vm->pool);
            if(!args)return 1;
            obj_t *obj = obj_pool_add_fun(vm->pool,
                module_name, sym, args);
            if(!obj)return 1;
            obj_t box;
            obj_init_box(&box, obj);
            if(!obj_frame_push(frame, &box))return 1;
        }else if(inst == vm->sym_apply){
            OBJ_STACKCHECK(2)
            obj_t *fun = OBJ_RESOLVE(OBJ_FRAME_NOS(frame));
            OBJ_TYPECHECK(fun, OBJ_TYPE_FUN)

            /* NOTE: we can't just use TOS as the head!
            Because obj_t on the stack are ephemeral.
            Gotta allocate a copy of it... */
            obj_t *head = obj_pool_objs_alloc(vm->pool, 1);
            if(!head)return 1;
            *head = *OBJ_FRAME_TOS(frame);

            obj_t *args = obj_pool_add_cell(vm->pool,
                head, OBJ_FUN_ARGS(fun));
            if(!args)return 1;

            OBJ_FUN_ARGS(fun) = args;
            frame->stack_tos--;
        }else if(inst == vm->sym_fun_module){
            OBJ_STACKCHECK(1)
            obj_t *fun = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(fun, OBJ_TYPE_FUN)
            obj_init_sym(OBJ_FRAME_TOS(frame), OBJ_FUN_MODULE_NAME(fun));
        }else if(inst == vm->sym_fun_name){
            OBJ_STACKCHECK(1)
            obj_t *fun = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(fun, OBJ_TYPE_FUN)
            obj_init_sym(OBJ_FRAME_TOS(frame), OBJ_FUN_DEF_NAME(fun));
        }else if(inst == vm->sym_fun_args){
            OBJ_STACKCHECK(1)
            obj_t *fun = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK(fun, OBJ_TYPE_FUN)
            obj_init_box(OBJ_FRAME_TOS(frame), OBJ_FUN_ARGS(fun));
        }else if(inst == vm->sym_ret){
            /* Pop all frame's blocks */
            while(frame->block_list){
                if(!obj_frame_pop_block(vm, frame))return 1;
            }
            goto skip_block_code_update;
        }else if(inst == vm->sym_vars){
            OBJ_FRAME_NEXT(var_lst)
            OBJ_TYPECHECK_LIST(var_lst)
            int n_vars = OBJ_LIST_LEN(var_lst);
            OBJ_STACKCHECK(n_vars)
            for(int i = n_vars - 1; i >= 0; i--){
                obj_t *obj_var = OBJ_HEAD(var_lst);
                OBJ_TYPECHECK(obj_var, OBJ_TYPE_SYM)
                obj_sym_t *var = OBJ_SYM(obj_var);
                obj_t *val = OBJ_FRAME_GET(frame, i);
                if(!obj_frame_set_var(frame, var, val))return 1;
                var_lst = OBJ_TAIL(var_lst);
            }
            frame->stack_tos -= n_vars;
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
        }else if(inst == vm->sym_error){
            OBJ_STACKCHECK(1)
            fprintf(stderr, "%s: Error: ", __func__);
            obj_fprint(OBJ_FRAME_TOS(frame), stderr, 2);
            putc('\n', stderr);
            return 1;
        }else if(inst == vm->sym_if){
            OBJ_FRAME_NEXT(ifcode)
            OBJ_TYPECHECK_LIST(ifcode)
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            bool b = OBJ_BOOL(OBJ_FRAME_TOS(frame));
            frame->stack_tos--;
            if(b){
                if(!obj_frame_push_block(
                    vm, frame, OBJ_BLOCK_BASIC, ifcode))return 1;
            }
        }else if(inst == vm->sym_ifelse){
            OBJ_FRAME_NEXT(ifcode)
            OBJ_FRAME_NEXT(elsecode)
            OBJ_TYPECHECK_LIST(ifcode)
            OBJ_TYPECHECK_LIST(elsecode)
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            bool b = OBJ_BOOL(OBJ_FRAME_TOS(frame));
            frame->stack_tos--;
            if(b){
                if(!obj_frame_push_block(
                    vm, frame, OBJ_BLOCK_BASIC, ifcode))return 1;
            }else{
                if(!obj_frame_push_block(
                    vm, frame, OBJ_BLOCK_BASIC, elsecode))return 1;
            }
        }else if(inst == vm->sym_and || inst == vm->sym_or){
            OBJ_FRAME_NEXT(ifcode)
            OBJ_TYPECHECK_LIST(ifcode)
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            bool b = OBJ_BOOL(OBJ_FRAME_TOS(frame));
            bool is_and = inst == vm->sym_and;
            if(is_and && b || !is_and && !b){
                frame->stack_tos--;
                if(!obj_frame_push_block(
                    vm, frame, OBJ_BLOCK_BASIC, ifcode))return 1;
            }else{
                obj_init_bool(OBJ_FRAME_TOS(frame), is_and? false: true);
            }
        }else if(inst == vm->sym_do){
            OBJ_FRAME_NEXT(inst_code)
            OBJ_TYPECHECK_LIST(inst_code)
            if(!obj_frame_push_block(
                vm, frame, OBJ_BLOCK_DO, inst_code))return 1;
        }else if(inst == vm->sym_for){
            OBJ_FRAME_NEXT(block_code)
            OBJ_FRAME_NEXT(inst_code)
            OBJ_TYPECHECK_LIST(inst_code)
            OBJ_TYPECHECK_LIST(block_code)
            if(!obj_frame_push_block(
                vm, frame, OBJ_BLOCK_FOR, inst_code))return 1;
            frame->block_list->u.o = block_code;
        }else if(inst == vm->sym_int_for){
            OBJ_FRAME_NEXT(inst_code)
            OBJ_TYPECHECK_LIST(inst_code)
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_INT)
            if(!obj_frame_push_block(
                vm, frame, OBJ_BLOCK_INT_FOR, inst_code))return 1;
            frame->block_list->u.int_for.i = 0;
            frame->block_list->u.int_for.n = OBJ_INT(OBJ_FRAME_TOS(frame));
            frame->stack_tos--;
        }else if(inst == vm->sym_list_for){
            OBJ_FRAME_NEXT(inst_code)
            OBJ_TYPECHECK_LIST(inst_code)
            OBJ_STACKCHECK(1)
            obj_t *list = OBJ_RESOLVE(OBJ_FRAME_TOS(frame));
            OBJ_TYPECHECK_LIST(list)
            if(!obj_frame_push_block(
                vm, frame, OBJ_BLOCK_LIST_FOR, inst_code))return 1;
            frame->block_list->u.o = list;
            frame->stack_tos--;
        }else if(inst == vm->sym_next){
            /* Find first loopy block */
            while(frame->block_list->type == OBJ_BLOCK_BASIC){
                if(!obj_frame_pop_block(vm, frame))return 1;
                if(!frame->block_list){
                    fprintf(stderr, "%s: Not allowed at toplevel: ",
                        __func__);
                    obj_sym_fprint(inst, stderr);
                    putc('\n', stderr);
                    return 1;
                }
            }

            if(frame->block_list->type == OBJ_BLOCK_DO){
                /* Jump to start of block */
                frame->block_list->code = frame->block_list->code_start;
            }else{
                /* Skip to end of block, since for and int_for need to
                do special stuff every time block is restarted */
                frame->block_list->code = &vm->pool->nil;
            }
            goto skip_block_code_update;
        }else if(inst == vm->sym_break){
            /* Find first loopy block */
            while(frame->block_list->type == OBJ_BLOCK_BASIC){
                if(!obj_frame_pop_block(vm, frame))return 1;
                if(!frame->block_list){
                    fprintf(stderr, "%s: Not allowed at toplevel: ",
                        __func__);
                    obj_sym_fprint(inst, stderr);
                    putc('\n', stderr);
                    return 1;
                }
            }

            /* Pop the loopy block */
            if(!obj_frame_pop_block(vm, frame))return 1;
            goto skip_block_code_update;
        }else if(inst == vm->sym_while){
            OBJ_STACKCHECK(1)
            OBJ_TYPECHECK(OBJ_FRAME_TOS(frame), OBJ_TYPE_BOOL)
            bool b = OBJ_BOOL(OBJ_FRAME_TOS(frame));
            frame->stack_tos--;
            if(!b){
                /* Find first loopy block */
                while(frame->block_list->type == OBJ_BLOCK_BASIC){
                    if(!obj_frame_pop_block(vm, frame))return 1;
                    if(!frame->block_list){
                        fprintf(stderr, "%s: Not allowed at toplevel: ",
                            __func__);
                        obj_sym_fprint(inst, stderr);
                        putc('\n', stderr);
                        return 1;
                    }
                }

                /* Pop the loopy block */
                if(!obj_frame_pop_block(vm, frame))return 1;
                goto skip_block_code_update;
            }
        }else if(inst == vm->sym_p_stack)obj_frame_dump_stack(frame, stderr, 0);
        else if(inst == vm->sym_p_vars)obj_frame_dump_vars(frame, stderr, 0);
        else if(inst == vm->sym_p_blocks)obj_frame_dump_blocks(frame, stderr, 0);
        else if(inst == vm->sym_p_frame)obj_frame_dump(frame, stderr, 0, true);
        else{
            fprintf(stderr, "%s: Unrecognized instruction: ", __func__);
            obj_sym_fprint(inst, stderr);
            putc('\n', stderr);
            return 1;
        }
    }else if(
        inst_obj_type == OBJ_TYPE_CELL ||
        inst_obj_type == OBJ_TYPE_NIL
    ){
        if(!obj_frame_push_block(
            vm, frame, OBJ_BLOCK_BASIC, inst_obj))return 1;
    }else{
        if(!obj_frame_push(frame, inst_obj))return 1;
    }

    block->code = code;
skip_block_code_update:
    return 0;
#   undef OBJ_STACKCHECK
#   undef OBJ_TYPECHECK
#   undef OBJ_TYPECHECK_LIST
#   undef OBJ_FRAME_NEXT
#   undef OBJ_FRAME_NEXTSYM
#   undef OBJ_FRAME_BINOP
}

