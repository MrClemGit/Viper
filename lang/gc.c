#include "lang.h"

/* defined in linker script */
#ifndef SIMULATOR
extern uint8_t __heap_base__[];
extern uint8_t __heap_end__[];
#else
extern uint8_t *__heap_base__;
extern uint8_t *__heap_end__;
#endif

#define GC_HEAPSIZE 32768
#define GC_HEAPS ((VM_RAM/GC_HEAPSIZE)+((VM_RAM%GC_HEAPSIZE)?(1):(0)))
#define PNTD(a) ((uint32_t)(a)-(uint32_t)(hbase))
#define PNT(x) ((PObject*)(x))
#define TNP(x) ((uint32_t)(x))


uint8_t *hbase;
uint8_t *hend;
uint8_t *hedge;
uint32_t hfreemem;
uint32_t hblocks;
uint32_t hfblocks;
uint32_t last_collect;
static PObject *_gcheaps[GC_HEAPS];
static PObject *_phead;
static uint16_t heapblocks[GC_HEAPS];
volatile uint8_t gc_checktime = 0;
static VSysTimer gc_vt;
uint32_t gc_period = TIME_U(GC_CHECKTIME, MILLIS);

void do_gctime(void *p) {
    (void)p;
    gc_checktime = 1;
    //SYSLOCK_I();
    //chVTSetI(&gc_vt, MS2ST(gc_period), do_gctime, NULL);
    //SYSUNLOCK_I();
}



void gc_init(void) {
    hbase = (uint8_t *)GC_ALIGN_SIZE((uint32_t)&__heap_base__);
    hend = (uint8_t *)GC_ALIGN_PREV_SIZE((uint32_t)&__heap_end__);
    hfreemem = (uint32_t)(hend - hbase);
    hedge = hbase;
    GCH_SIZE_SET(PNT(hedge), 0xffff);
    hblocks = 0;
    hfblocks = 1;
    _phead = NULL;
    memset(_gcheaps, 0, sizeof(_gcheaps));
    memset(heapblocks, 0, sizeof(heapblocks));
    GCH_NEXT_SET(hbase, hbase);
    //debug("GC Init:\r\n");
    //debug("- start at: %x\r\n", hbase);
    //debug("-   end at: %x\r\n", hend);
    //debug("-    heaps: %i %i\n", GC_HEAPS, sizeof(_gcheaps));
}

void gc_start() {
    gc_vt = vosTimerCreate();
    vosTimerRecurrent(gc_vt, gc_period, do_gctime, NULL);
    //chVTSet(&gc_vt, MS2ST(gc_period), do_gctime, NULL);
}


//REMEBER TO ZERO EVERYTHING!!
PObject *gc_alloc(uint8_t type, uint8_t flags, uint16_t size, uint8_t onheap) {
    uint16_t tsize;
    uint16_t bfree;
    PObject *obj = NULL, *prev, *cur, *next;


    gc_wait();


    PThread *pth = PTHREAD_CURRENT();
#if VM_DEBUG
    int32_t thid = (pth) ? ((int32_t)(vosThGetId(pth->th))) : -1;
#endif
    tsize = GC_ALIGN_SIZE(size);//size +(GC_ALIGNMENT- size % GC_ALIGNMENT);

    debug( "> th (%i): >>>>ALLOC %i of %i [f:%i,t:%i,h:%i]\r\n", thid, type, size, hfreemem, tsize, heapblocks);

    if (hfreemem < tsize) {
        /* no space, fail */
        goto exit_alloc;
    } else {
        /* normal mark & sweep */
        if (gc_checktime || hfreemem < GC_TRIGGER_MIN) {
            info( "> th (%i) %x: >>> COLLECTING\r\n", thid, pth);
            gc_mark();
            gc_sweep();
            last_collect = _systime_millis;
            gc_checktime = 0;
        }
        next = cur = GCH_NEXT(hedge);
        prev = PNT(hedge);

        do {
            bfree = GCH_SIZE(cur);
            debug( "* check block %x/%x/%x of size %i/%i\n", cur, prev, hedge, bfree, tsize);
            if (bfree >= tsize) {
                /*found free block: split or give whole */
                obj = cur;
                if (bfree - tsize < 16) {
                    debug( "* found full block %x/%x/%x next is %x\n", cur, prev, hedge, GCH_NEXT(cur));
                    /* don't split */
                    tsize = bfree;
                    cur = GCH_NEXT(cur);
                    hfblocks--;
                } else {
                    /* split */
                    debug( "* found splittable block %x/%x/%x new is %x\n", cur, prev, hedge, PNT(TNP(cur) + tsize));
                    cur = PNT(TNP(cur) + tsize); //(PObject *)(((uint8_t *)cur) + tsize);
                    if (obj == PNT(hedge)) {
                        debug( "* block is hedge, new hedge is %x\n", cur);
                        hedge = (uint8_t *)cur;
                        GCH_SIZE_SET(PNT(hedge), MIN(0xffff, (uint32_t)(hend - hedge)));
                        if (prev == obj) {
                            //set to self
                            debug( "* block is hedge, no other free blocks\n");
                            GCH_NEXT_SET(PNT(hedge), PNT(hedge));
                            //prev = PNT(hedge);
                            break;
                        } else {
                            //set to next of hedge before alloc
                            debug( "* block is hedge, other free blocks, prev is %x so next is still %x\n", prev, GCH_NEXT(obj));
                            GCH_NEXT_SET(PNT(hedge), GCH_NEXT(obj));
                        }
                    } else {
                        debug( "* block is not hedge: next is %x\n", GCH_NEXT(obj));
                        GCH_NEXT_SET(cur, GCH_NEXT(obj));
                        GCH_SIZE_SET(cur, bfree - tsize); //if cur==hedge np
                    }
                }
                //set prev to current
                debug( "* block allocated, update prev %x setting next to %x\n", prev, cur);
                GCH_NEXT_SET(prev, cur);
                break;
            } else {
                prev = cur;
                cur = GCH_NEXT(cur);
            }
        } while (cur != next);
    }
    if (obj == NULL) {
        goto exit_alloc;
    }

    //zero everything
    memset(((uint8_t *)obj), 0, tsize);

    //set on stage
    if (onheap) {
        //allocate on the heap list
        uint16_t heapn = (TNP(obj) - TNP(hbase)) / GC_HEAPSIZE;
        PObject *gcheap = _gcheaps[heapn];
        if (!gcheap) {
            _gcheaps[heapn] = obj;
            GCH_HEAP_NEXT_SET(obj, obj);
            GCH_HEAP_PREV_SET(obj, obj);
            debug( "alloc %x first on heap %i @ %x\n", obj, heapn, obj);
        } else {
            //gcheap = _gcheaps[heapn];
            next = GCH_HEAP_NEXT(gcheap);
            GCH_HEAP_PREV_SET(next, obj);
            GCH_HEAP_NEXT_SET(gcheap, obj);
            GCH_HEAP_PREV_SET(obj, gcheap);
            GCH_HEAP_NEXT_SET(obj, next);
            debug( "alloc %x,on heap %i @ %x with next %x prev %x\n", obj, heapn, gcheap, next, gcheap);
        }
        heapblocks[heapn]++;
    } else {
        //allocate to system or thread
        if (pth) {
            GCH_NEXT_SET(obj, pth->stage);
            pth->stage = obj;
        }
        GCH_FLAG_SET(obj, GC_STAGED);
    }

    //set gc data
    GCH_SIZE_SET(obj, tsize);
    obj->header.type = type;
    obj->header.flags = flags;

    //statistics
    hfreemem -= tsize;
    hblocks++;
    debug( "alloced %x -> %x <- %x\n", obj, GCH_HEAP_NEXT(obj), GCH_HEAP_PREV(obj));


    debug( "> th (%i)%x: end alloc %x->%x %x %x edge %i\n", thid, pth, obj, GCH_NEXT(obj), GCH_FLAG(obj), obj->header.gch.flags, PNTD(hedge));
exit_alloc:
    //TODO: obj == NULL --> raise system exception: out of memory!
    //debug(">>>>>ALLOCED %i\r\n", obj);
    //debug("o %i\n",obj ? 1:0);
    gc_signal();
    //gc_trace();
    return obj;
}



void *gc_malloc(uint32_t sz) {
    PBuffer *buf = ALLOC_OBJ_HEAP(PBuffer, PBUFFER, 1, sz);
    debug( "MALLOC %x of %i [%x,%i]\n", buf, sz, _gcheaps, heapblocks);
    //gc_trace();
    return (void *)(buf->buffer);
}

void *gc_realloc(void *pnt, uint32_t sz) {
    PBuffer *old = (PBuffer *)(( (uint8_t *)pnt) - sizeof(PObjectHeader));
    PBuffer *buf = ALLOC_OBJ_HEAP(PBuffer, PBUFFER, 1, sz);
    int csz = PBUFFER_SIZE(old) < sz  ? PBUFFER_SIZE(old) : sz;
    memcpy(buf->buffer, pnt, csz);
    debug( "REALLOC %x of %i from %x of %i cpd %i [%x,%i]\n", buf, sz, old, PBUFFER_SIZE(old), csz, _gcheaps, heapblocks);
    gc_free(old->buffer);
    return (void *)(buf->buffer);
}

void _gc_free(void *pnt) {
    PObject *obj = (PObject *)(( (uint8_t *)pnt) - sizeof(PObjectHeader));
    uint16_t heapn = ((uint32_t)obj - (uint32_t)hbase) / GC_HEAPSIZE;
    PObject *hcur = _gcheaps[heapn];
    debug( "FREE %x of %i [%x,%i]\n", obj, PBUFFER_SIZE(obj), hcur, heapblocks[heapn]);

    PObject *hnext = GCH_HEAP_NEXT(obj);
    PObject *hprev = GCH_HEAP_PREV(obj);

    debug( "freeing %X with mem %i and hblk %i next %x and prev %x gcheap[%i] %x \n", obj, hfreemem, heapblocks[heapn], hnext, hprev, heapn, hcur);

    if (hnext == obj) {
        //one element heap
        _gcheaps[heapn] = NULL;
    } else {
        GCH_HEAP_NEXT_SET(hprev, hnext);
        GCH_HEAP_PREV_SET(hnext, hprev);
        if (hcur == obj)
            _gcheaps[heapn] = hnext;
    }

    GC_UNSTAGE(obj);
    heapblocks[heapn]--;
}

void gc_free(void *pnt) {
    gc_wait();
    _gc_free(pnt);
    gc_signal();
}


#define GC_KEEP(o) gc_keep((PObject**)&(o),1)
#define GC_KEEP_NOTNULL(o)  if(o) gc_keep((PObject**)&(o),1)
#define GC_KEEP_MANY(o,n) gc_keep((PObject**)o,n);
#define GC_MARK(o) GCH_FLAG_SET(o,GC_USED_MARKED)


void gc_keep(PObject **objs, uint32_t nobjs) {
    while (nobjs--) {
        PObject *obj = objs[nobjs];
        if (obj) {
            if (!IS_TAGGED(obj)) {
mark_it:
                if (GCH_FLAG(obj) == GC_USED) {
                    GCH_NEXT_SET2(obj, _phead);
                    _phead = obj;
                    //mark it! if it is kept again, it will not be added
                    GC_MARK(obj);
                }
            } else {
                if (TAGTYPE(obj) == PCELL) {
                    debug("marking cell %x %x\n", obj, PCELL_ARG(obj));
                    //vosThSleep(TIME_U(1,SECONDS));
                    obj = PCELL_ARG(obj);
                    goto mark_it;
                }
            }
        }
        // if (obj && !IS_TAGGED(obj) && GCH_FLAG(obj) == GC_USED) {
        //     GCH_NEXT_SET2(obj, _phead);
        //     _phead = obj;
        //     //mark it! if it is kept again, it will not be added
        //     GC_MARK(obj);
        // }
    }
}

void gc_keep_cells(PTuple *closure) {
    PObject **objs = PSEQUENCE_OBJECTS(closure);
    uint32_t nobjs = PSEQUENCE_ELEMENTS(closure);
    GC_MARK(closure);//mark: closure must not be gc'ed as a normal tuple, no "objects" inside
    while (nobjs--) {
        PObject *obj = objs[nobjs];
        if (obj) {
            debug("keeping cell %x => %x\n", obj, PCELL_ARG(obj));
            obj = PCELL_ARG(obj);
            GC_KEEP(obj);//PCELL_ARG(obj) is a freevar tuple, keep it

        }
    }
}


void gc_collect() {
    gc_wait();
    gc_mark();
    gc_sweep();
    last_collect = _systime_millis;
    gc_checktime = 0;
    gc_signal();
}



void gc_keep_root(void) {
    PObject *obj;
    int i;

    if (_vmpnt) {
        //get the root set
        obj = PNT(_vm.thlist);
        do {
            PThread *pth = (PThread *)obj;
            VThread vth = pth->th;
            debug( "Scanning thread %x %i\n", pth, pth->header.flags);
            if (vth && vosThGetStatus(vth) == VTHREAD_INACTIVE) {
                //terminated, remove from list...it will die if no references remain
                pth->prev->next = pth->next;
                pth->next->prev = pth->prev;
                GCH_FLAG_SET(pth, GC_USED);
                //free workspace
                //if PThread has references, it's ok. But it can't be restarted
                _gc_free(vth);
                pth->th = NULL;
            } else {
                GC_MARK(pth);
                if (pth->frame) GC_KEEP(pth->frame);
            }
            obj = PNT(pth->next);
        } while (obj != PNT(_vm.thlist));

        for (i = 0; i < _vm.nmodules; i++) {
            PModule *mod = VM_MODULE(i);
            GC_MARK(mod);
            if (PMODULE_IS_LOADED(mod)) {
                GC_KEEP_MANY(mod->globals, mod->nfo.nums.nglobals);
            } else {
                GC_KEEP(mod->nfo.frame);
            }
        }
        //-----> irqs could be modifing irq struct here: no worries
        SYSLOCK();
        //slots & irqstack
        for (i = 0; i < _vm.irqn; i++) {
            GC_KEEP(_vm.irqstack[i].fn);
            GC_KEEP_NOTNULL(_vm.irqstack[i].args);
        }

        for (i = 0; i < EXT_SLOTS; i++) {
            if (_vm.irqslots[0][i].fn) {
                GC_KEEP(_vm.irqslots[0][i].fn);
                GC_KEEP_NOTNULL(_vm.irqslots[0][i].args);
            }
            if (_vm.irqslots[1][i].fn) {
                GC_KEEP(_vm.irqslots[1][i].fn);
                GC_KEEP_NOTNULL(_vm.irqslots[1][i].args);
            }
        }
        if (_vm.irqcur) {
            GC_KEEP(_vm.irqcur->fn);
            GC_KEEP_NOTNULL(_vm.irqcur->args);
        }


        if (_vm.timers) {
            obj = _vm.timers;
            do {
                GC_KEEP(obj);
                obj = (PObject *)((PSysObject *)obj)->sys.timer.next;
            } while (obj != _vm.timers);
        }


        SYSUNLOCK();
        //-----> irq could be modifing irq struct here:
        //       - it can add something to irqslots: np, the objects added must be in an active frame (gc lock is on, no new frame can be created)
        //       - it can add something to irqstack from pin irq: np, the objects are sitting in irqslots
        //       - it can add something to irqstack from timers: that timer was in the timer list, so it is already kept
        //       - it can be the irqthread at 2 different times:
        //           - it modifies _vm.irqcur after putting it in a new frame: if happens up there, np. if happens now, already kept
        //           - it does _vm.irqn--: if happens up there, the removed irqfn is in irqcur. if happens here, already kept
    }

}


//TODO: remove the stack and make a list of used/marked via header!! phead vs ptail
void gc_mark() {
    int i;
    PObject *obj;

    debug( "\n\n>>>>>>>MARK started %x\n", _phead);

    gc_trace();
    gc_keep_root();

    while (_phead) {
        obj = _phead;
        _phead = GCH_NEXT(obj);

        int tt = PTYPE(obj);
        switch (tt) {
            case PBYTEARRAY:
            case PSHORTARRAY:
                if (_PMS(obj)->seq)
                    GC_MARK( _PMS(obj)->seq);
                break;
            case PLIST:
            case PTUPLE: {
                i = PSEQUENCE_ELEMENTS(obj);
                PObject **objs = PSEQUENCE_OBJECTS(obj);
                if (objs) {
                    GC_KEEP_MANY(objs, i);
                    if (PSEQUENCE_BUFFER(obj)) {
                        GC_MARK(PSEQUENCE_BUFFER(obj));
                    }
                }
            }
            break;

            case PFSET:
            case PSET:
            case PDICT: {
                PDict *tmp = (PDict *)obj;
                int e = 0;
                HashEntry *ee;
                while ( (ee = phash_getentry(tmp, e++)) != NULL ) {
                    GC_KEEP_NOTNULL(ee->key);
                    if (tt == PDICT)
                        GC_KEEP_NOTNULL(ee->value);
                }
                if (tmp->entry)
                    GC_MARK(tmp->entry);
            }
            break;

            case PFUNCTION: {
                PFunction *tmp = (PFunction *)obj;
                if (tmp->defargs) GC_KEEP_MANY(tmp->storage, tmp->defargs);
                //TODO: the following can be optimized by avoiding the check on names...
                //and use macros to access function fields for portability...
                if (tmp->defkwargs) GC_KEEP_MANY(tmp->storage + tmp->defargs, 2 * tmp->defkwargs);
                if (PCODE_CELLVARS(PCODE_MAKE(tmp->codeobj))) {
                    obj = (PObject *)PFUNCTION_GET_CLOSURE(tmp);
                    GC_KEEP(obj);
                    //if (obj)
                    //   gc_keep_cells((PTuple *)obj);
                }
            }
            break;

            case PMETHOD: {
                PMethod *tmp = (PMethod *)obj;
                GC_KEEP(tmp->self);
                GC_KEEP(tmp->fn);
            }
            break;

            case PCLASS: {
                PClass *tmp = (PClass *)obj;
                GC_KEEP(tmp->bases);
                GC_KEEP(tmp->dict);
            }
            break;

            case PINSTANCE: {
                PInstance *tmp = (PInstance *)obj;
                GC_KEEP(tmp->base);
                GC_KEEP(tmp->dict);
            }
            break;

            case PITERATOR:
                GC_KEEP(   ((PIterator *)obj)->iterable );
                break;

            case PFRAME: {
                //debug("checking frame %i\n", obj);
                PFrame *tmp = (PFrame *)obj;
                PCode *code = PCODE_MAKE(tmp->code);
                GC_KEEP(tmp->parent);
                //GC_KEEP(tmp->block);
                GC_KEEP_MANY(PFRAME_PSTACK(tmp), tmp->sp);
                GC_KEEP_MANY(PFRAME_PLOCALS(tmp, code), code->nlocals);
                if (PCODE_HASVARS(code))
                    GC_KEEP_MANY(PFRAME_VARS(tmp, code), 2);

                /*if (PCODE_FREEVARS(code)) {
                    obj = PFRAME_TFREEVARS(tmp,code);//(PObject *)PFRAME_FREEVARS(tmp);
                    debug("keeping freevars %x for %x\n", obj, tmp);
                    GC_KEEP(obj);
                }
                debug("FRAME %x %i %i\n", tmp, PCODE_CELLVARS(code), PCODE_FREEVARS(code));
                if (PCODE_CELLVARS(code)) {
                    obj = (PObject *)PFRAME_CELLVARS(tmp);
                    debug("keeping cells for %x\n", obj);
                    GC_KEEP(obj);
                    //if (obj)
                    //    gc_keep_cells((PTuple *)obj);
                }
                */
            }
            break;
            /*case PBLOCK:
                //debug("checking block %i\n", obj);
                GC_KEEP(((PBlock *)obj)->next);
                break;
            */
            case PSYSOBJ: {
                PSysObject *tmp = (PSysObject *) obj;
                SYSLOCK();
                if (tmp->type == PSYS_TIMER) {
                    GC_KEEP(tmp->sys.timer.fn.fn);
                    GC_KEEP(tmp->sys.timer.fn.args);
                }
                SYSUNLOCK();
            }
            break;

            default:
                break;
        }
        //no need to mark obj. It has already been marked in gc_keep
        //GC_MARK(obj);
    }

    gc_trace();
    _phead = NULL;
    obj = PNT(_vm.thlist);
    do {
        PThread *pth = (PThread *)obj;
        debug( "Scanning thread %x %i for consts %x %i\n", pth, pth->header.flags, pth->cocache, pth->cachesize);
        for (i = 0; i < pth->cachesize; i++) {
            PObject *co = pth->cocache[i].obj;

            if (co && GCH_FLAG(co) != GC_USED)
                continue;
            //not marked, not staged, not null, not const_marked: remove it
            pth->cocache[i].obj = NULL;
        }
        obj = PNT(pth->next);
    } while (obj != PNT(_vm.thlist));
    debug( ">>>>>>>MARK stopped\n\n\n");

}




//#define GC_PUSH(o) (*stack_top=(PObject*)(o),stack_top--)
//#define GC_POP() (*++stack_top)

#define GC_PUSH(o) (stack_base[sp]=(PObject*)(o),sp--)
#define GC_POP() (sp++,stack_base[sp])

#define TMP_VAR(type,o) type *tmp =(type *)(o)
#define SPW(a) (a)
//((((uint8_t*)stack_base)-((uint8_t*)(a)))/GC_ALIGNMENT)


void gc_mark_old(void) {
    /*
    int i;
    PObject **stack_base = ((PObject **) hend);
    PObject **stack_end = ((PObject **) (hedge + sizeof(PObjectHeader)));
    PObject *obj;
    int32_t sp = -1;
    int32_t tsp = -((((uint32_t)stack_base) - ((uint32_t)stack_end)) / sizeof(PObject *));

    hfreemem = (uint32_t)(hend - hbase);
    debug("\n\n>>>>>>>MARK started %i/%i\n", sp, tsp);

    gc_trace();
    // mark: TODO, check if stack can hold root set
    // TODO: check stack overflow
    obj = _vm.thlist;
    do {
        PThread *pth = (PThread *)obj;
        debug("Scanning thread %x %i\n", pth, pth->header.flags);
        if (pth->header.flags == 1) {
            //terminated, remove from list...it will die if no references remain
            pth->prev->next = pth->next;
            pth->next->prev = pth->prev;
            GCH_FLAG_SET(pth, GC_USED);
        } else {
            GCH_FLAG_SET(pth, GC_USED_MARKED);
            if (pth->frame) GC_PUSH(pth->frame);
            if (pth->wks) {
                GCH_FLAG_SET(pth->wks, GC_USED_MARKED);
            }
        }
        obj = pth->next;
    } while (obj != _vm.thlist);

    GCH_FLAG_SET(P_ONE_F, GC_USED_MARKED);
    GCH_FLAG_SET(P_ZERO_F, GC_USED_MARKED);
    GCH_FLAG_SET(P_M_ONE_F, GC_USED_MARKED);
    GCH_FLAG_SET(P_EMPTY_S, GC_USED_MARKED);
    GCH_FLAG_SET(P_SPACE_S, GC_USED_MARKED);
    GCH_FLAG_SET(P_NEWLINE_S, GC_USED_MARKED);

    for (i = 0; i < _vm.nmodules; i++) {
        PModule *mod = VM_MODULE(i);
        GCH_FLAG_SET(mod, GC_USED_MARKED);
        if (PMODULE_IS_LOADED(mod)) {
            int j;
            for (j = 0; j < mod->nfo.nums.nglobals; j++) {
                if (mod->globals[j] && !IS_TAGGED(mod->globals[j]))
                    GC_PUSH(mod->globals[j]);
            }
        } else {
            GC_PUSH(mod->nfo.frame);
        }
    }

    SYSLOCK();
    //slots & irqstack
    for (i = 0; i < _vm.irqn; i++) {
        GC_PUSH(_vm.irqstack[i].fn);
        GC_PUSH(_vm.irqstack[i].args);
    }

    for (i = 0; i < EXT_SLOTS; i++) {
        if (_vm.irqslots[i].fn) {
            GC_PUSH(_vm.irqslots[i].fn);
            GC_PUSH(_vm.irqslots[i].args);
        }
    }
    if (_vm.irqcur) {
        GC_PUSH(_vm.irqcur->fn);
        GC_PUSH(_vm.irqcur->args);
    }
    SYSUNLOCK();

    while (sp >= tsp && sp < -1) {
        //debug("popping at %i %i\n", SPW(sp), sp);
        obj = GC_POP();
        //debug("considering %x %i/%i\n", obj, sp, tsp);

        if (IS_TAGGED(obj) || GCH_FLAG(obj) == GC_USED_MARKED)
            continue;
        //debug("flg was %i\n", GCH_FLAG(obj));
        hfreemem -= GCH_SIZE(obj);
        //debug("marking %s %i @ %x\n", typestrings[PTYPE(obj)], GCH_SIZE(obj), obj);
        switch (PTYPE(obj)) {
            case PBYTEARRAY:
                //debug("checking bytearray %i\n", obj);
                if (_PMS(obj)->seq)
                    GCH_FLAG_SET( _PMS(obj)->seq, GC_USED_MARKED);
                break;
            case PLIST: {
                //debug("checking list %i\n", obj);
                TMP_VAR(PList, obj);
                if (tmp->seq)
                    GCH_FLAG_SET(tmp->seq, GC_USED_MARKED);
                for (i = 0; i < tmp->elements; i++) {
                    if (!IS_TAGGED(POBJ_BUFFER(tmp->seq)[i])) {
                        //debug("...pushing item[%i] %i at %i\n", i, POBJ_BUFFER(tmp->seq)[i], SPW(sp));
                        GC_PUSH(POBJ_BUFFER(tmp->seq)[i]);
                    }
                }
            }
            break;

            case PTUPLE: {
                //debug("checking tuple %i\n", obj);
                TMP_VAR(PTuple, obj);
                for (i = 0; i < tmp->elements; i++) {
                    if (!IS_TAGGED(POBJ_ISEQ(tmp)[i])) {
                        //debug("...pushing item[%i] %i at %i\n", i, POBJ_ISEQ(tmp)[i], SPW(sp));
                        GC_PUSH(POBJ_ISEQ(tmp)[i]);
                    }
                }
            }
            break;
            case PRANGE:
                break;

            case PFSET:
                // TODO
                break;

            case PSET:
                // TODO
                break;

            case PDICT: {
                TMP_VAR(PDict, obj);
                int e = 0;
                DictEntry *ee;
                while (ee = pdict_getentry(tmp, e++)) {
                    if (ee->key && !IS_TAGGED(ee->key)) {
                        GC_PUSH(ee->key);
                        //debug("dict_kpush: %i\r\n",ee->key);
                    }
                    if (ee->value && !IS_TAGGED(ee->value)) {
                        GC_PUSH(ee->value);
                        //debug("dict_vpush: %i\r\n",ee->value);
                    }
                }
                if (tmp->entry)
                    GCH_FLAG_SET(tmp->entry, GC_USED_MARKED);
            }
            break;

            case PFUNCTION:
                //TODO: function defargs!!!
                break;

            case PMETHOD: {
                TMP_VAR(PMethod, obj);
                GC_PUSH(tmp->self);
                if (!IS_TAGGED(tmp->fn))
                    GC_PUSH(tmp->fn);
            }
            break;

            case PCLASS: {
                TMP_VAR(PClass, obj);
                //debug("Marking CLASS %i %i %i\r\n",tmp, tmp->bases,tmp->dict);
                if (tmp->bases)
                    GC_PUSH(tmp->bases);
                if (tmp->dict)
                    GC_PUSH(tmp->dict);
            }
            break;

            case PINSTANCE: {
                TMP_VAR(PInstance, obj);
                if (tmp->base)
                    GC_PUSH(tmp->base);
                if (tmp->dict)
                    GC_PUSH(tmp->dict);
            }
            break;

            case PBUFFER:
                // should not get here
                break;

            case PSLICE:
                // TODO
                break;

            case PITERATOR:
                //debug("checking iterator %i\n", obj);
                //debug("...pushing iterable %i at %i\n", ((PIterator *)obj)->iterable, SPW(sp));
                GC_PUSH(   ((PIterator *)obj)->iterable );
                break;

            case PFRAME: {
                //debug("checking frame %i\n", obj);
                TMP_VAR(PFrame, obj);
                PCode *code = PCODE_MAKE(tmp->code);
                if (tmp->parent) {
                    //debug("...pushing parent %i at %i\n", tmp->parent, SPW(sp));
                    GC_PUSH(tmp->parent);
                }
                if (tmp->block) {
                    //debug("...pushing block %i at %i\n", tmp->block, SPW(sp));
                    GC_PUSH(tmp->block);
                }

                for (i = 0; i < tmp->sp; i++) {
                    if (!IS_TAGGED(tmp->storage[i])) {
                        //debug("...pushing stack[%i] %i at %i\n", i, tmp->storage[i], SPW(sp));
                        GC_PUSH(tmp->storage[i]);
                    }
                }
                for (i = code->stacksize; i < code->stacksize + code->nlocals + code->nfree; i++) {
                    if (!IS_TAGGED(tmp->storage[i]) && tmp->storage[i]) {
                        //debug("...pushing var[%i] %i at %i\n", i - code->stacksize, tmp->storage[i], SPW(sp));
                        GC_PUSH(tmp->storage[i]);
                    }
                }
            }
            break;

            case PBLOCK:
                //debug("checking block %i\n", obj);
                if (  ((PBlock *)obj)->next) {
                    //debug("...pushing block %i at %i\n", ((PBlock *)obj)->next, SPW(sp));
                    GC_PUSH(((PBlock *)obj)->next);
                }
                break;

            case PSYSOBJ: {
                TMP_VAR(PSysObject, obj);
                SYSLOCK();
                if (tmp->type == PSYS_TIMER && tmp->sys.timer.fn.fn) {
                    GC_PUSH(tmp->sys.timer.fn.fn);
                    if (tmp->sys.timer.fn.args)
                        GC_PUSH(tmp->sys.timer.fn.args);
                }
                SYSUNLOCK();
            }
            break;

            default:
                break;
        }
        GCH_FLAG_SET(obj, GC_USED_MARKED);
    }
    if (sp < tsp) {
        //TODO: emergency! raise system exception
    }

    gc_trace();
    debug(">>>>>>>MARK stopped\n\n\n");
    */
}


void gc_sweep(void) {
    PObject *start = (PObject *)hbase;
    PObject *cur = start;
    PObject *prev = (PObject *)hedge;
    uint32_t flags;
    uint32_t prev_is_free = 0;


    debug( "\n\n<<<<<<<SWEEP started\n");

    /*heapwalk and create free list */
    hblocks = 0;
    hfblocks = 0;
    hfreemem = 0;
    while (cur != (PObject *)hedge) {
        flags = GCH_FLAG(cur);
        //debug("cur is %x with flag %i\n", cur, flags);
        switch (flags) {
            case GC_FREE:
            /*case GC_USED_UNMARKED:*/
gc_sweep_free:            
                hfreemem += GCH_SIZE(cur);
                debug("sweeping %x\n", cur);
                //debug("found free block: %i (%i,%i) %i\n",cur,hblocks,hfblocks,GCH_FLAG(cur));
                if (prev_is_free && (GCH_SIZE(prev) + GCH_SIZE(cur) <= 0xffff)) {
                    /* coalesce */
                    GCH_SIZE_SET(prev, GCH_SIZE(prev) + GCH_SIZE(cur));
                    //debug("...coalesced with prev %i~%i size %i\n",prev,cur,GCH_SIZE(prev));
                    prev_is_free = (GCH_SIZE(prev) < 0xffff);
                } else {
                    prev_is_free = (GCH_SIZE(cur) < 0xffff);
                    GCH_NEXT_SET(prev, cur); /*unset mark flags. First time, sets hedge->next */
                    prev = cur;
                    hfblocks++;
                    //debug("...splitted prev %i~%i size %i (%i,%i)\n",prev,cur,GCH_SIZE(prev),hblocks,hfblocks);
                }
                break;
            case GC_USED_UNMARKED:
                if (PHEADERTYPE(cur) == PSYSOBJ) {
                    if (((PSysObject *)cur)->type == PSYS_TIMER) {
                        //if we are here, timer is not in timers and not reachable...free vos mem
                        _gc_free( ((PSysObject *)cur)->sys.timer.vtm);
                    } else if (((PSysObject *)cur)->type == PSYS_SEMAPHORE){
                        //if it's a lost semaphore, still active, anything can happen :-o
                        _gc_free( ((PSysObject *)cur)->sys.sem);
                    }
                }
                goto gc_sweep_free;
            case GC_USED_MARKED:
                prev_is_free = 0;
                GCH_FLAG_SET(cur, GC_USED);
                hblocks++;
                //debug("found marked block: %i (%i,%i) %i\n",cur,hblocks,hfblocks,GCH_FLAG(cur));
                break;
            case GC_STAGED:
                /* don't touch flags */
                prev_is_free = 0;
                hblocks++;
                //debug("found staged block: %i (%i,%i) %i\n",cur,hblocks,hfblocks,GCH_FLAG(cur));
                break;
        }
        /* skip current */
        cur = (PObject *)(((uint8_t *)(cur)) + GCH_SIZE(cur));
    }

    if (prev_is_free) {
        /* coalesce with hedge */
        hfreemem -= GCH_SIZE(prev);
        GCH_NEXT_SET(prev, GCH_NEXT(hedge));
        hedge = (uint8_t *)prev;
        //debug("coalesced with hedge\n");
    } else {
        GCH_NEXT_SET(prev, hedge);
        //debug("not coalesced with hedge\n");
        hfblocks++;
    }
    debug( "S%i\n", hfreemem);
    hfreemem += ((uint32_t)hend - (uint32_t)hedge);
    debug( "S%i\n", hfreemem);
    GCH_SIZE_SET(PNT(hedge), MIN(0xffff, hfreemem));
    gc_trace();
    debug( "<<<<<<<<SWEEP stopped (%i,%i)\n\n\n", hblocks, hfblocks);

}


void gc_compact(void) {

    /*
        uint8_t *pfree;
        uint8_t *plive;
        PObject *obj;
        int i;

        SEMAPHORE_WAIT(gcsem);
        if (hlocked) {
            //can't compact :(
            chBSemSignal(&gcsem);
            return;

        }

        gc_mark();

        debug("\n\n>>>>>>>COMPACT started\n");

        pfree = hbase;
        plive = hbase;
        hfblocks = 1;
        hblocks = 0;

        //pass 1: calculate offsets
        while (plive != hedge) {
            obj = (PObject *) plive;
            if (GCH_FLAG(obj) >= GC_STAGED) {
                pfree += GCH_SIZE(obj);
                hblocks++;
            }
            plive += GCH_SIZE(obj);
        }

        //pass 2: change references
        plive = hbase;
        while (plive != hedge) {
            obj = (PObject *)plive;
            if (GCH_FLAG(obj) >= GC_STAGED) {
                switch (PTYPE(obj)) {
                    case PBYTEARRAY: {
                        TMP_VAR(PMutableSequence, obj);
                        GC_UPDATE_REF(tmp->seq, PBuffer);
                    }
                    break;
                    case PLIST: {
                        TMP_VAR(PMutableSequence, obj);
                        for (i = 0; i < tmp->elements; i++) {
                            if (!IS_TAGGED(POBJ_BUFFER(tmp->seq)[i]))
                                GC_UPDATE_REF(POBJ_BUFFER(tmp->seq)[i], PObject);
                        }
                        GC_UPDATE_REF(tmp->seq, PBuffer);
                    }
                    break;

                    case PTUPLE: {
                        TMP_VAR(PImmutableSequence, obj);
                        for (i = 0; i < tmp->elements; i++) {
                            if ( !IS_TAGGED(POBJ_ISEQ(tmp)[i])) {
                                GC_UPDATE_REF(POBJ_ISEQ(tmp)[i], PObject);
                            }
                        }
                    }
                    break;
                    case PRANGE:
                        break;

                    case PFSET:
                        // TODO
                        break;

                    case PSET:
                        // TODO
                        break;

                    case PDICT: {
                        TMP_VAR(PDict, obj);
                        int e = 0;
                        DictEntry *ee;
                        do {
                            ee = pdict_getentry(tmp, e++);
                            if (!IS_TAGGED(ee->key))
                                GC_UPDATE_REF(ee->key, PObject);
                            if (!IS_TAGGED(ee->value))
                                GC_UPDATE_REF(ee->value, PObject);
                        } while (ee);
                    }
                    break;

                    case PFUNCTION:
                        //TODO
                        break;

                    case PCLASS:
                        //
                        break;

                    case PINSTANCE:
                        //
                        break;

                    case PBUFFER:
                        //
                        break;

                    case PSLICE:
                        //
                        break;

                    case PITERATOR:
                        GC_UPDATE_REF(((PIterator *)obj)->iterable, PObject);
                        break;

                    case PFRAME: {
                        TMP_VAR(PFrame, obj);
                        PCode *code = PCODE_MAKE(tmp->code);
                        if (tmp->parent) {
                            GC_UPDATE_REF(tmp->parent, PFrame);
                        }
                        if (tmp->block) {
                            GC_UPDATE_REF(tmp->block, PBlock);
                        }

                        for (i = 0; i < tmp->sp; i++) {
                            if (!IS_TAGGED(tmp->storage[i])) {
                                GC_UPDATE_REF(tmp->storage[i], PObject);
                            }
                        }
                        for (i = code->stacksize; i < code->stacksize + code->nlocals + code->nfree; i++) {
                            if (!IS_TAGGED(tmp->storage[i]) && tmp->storage[i]) {
                                GC_UPDATE_REF(tmp->storage[i], PObject);
                            }
                        }
                    }
                    break;

                    case PBLOCK:
                        if (((PBlock *)obj)->next) {
                            GC_UPDATE_REF(((PBlock *)obj)->next, PBlock);
                        }
                        break;

                    default:
                        break;

                }
            }
        }


        // pass 3: move
        plive = hbase;
        while (plive != hedge) {
            obj = (PObject *)plive;
            if (GCH_FLAG(obj) >= GC_STAGED) {
                plive += GCH_SIZE(obj);
                GCH_FLAG_SET(obj, GC_USED);
                memmove(GCH_REF(obj), obj, GCH_SIZE(obj));
            }
        }

        // reset hedge
        hedge = plive;
        GCH_NEXT_SET(hedge, hedge);

        //gc_trace();
        debug("<<<<<<<<COMPACT stopped (%i,%i)\n\n\n", hblocks, hfblocks);

        SEMAPHORE_SIGNAL(gcsem);
        */
}

/*
void gc_dump(Stream *ps, int thid) {
    uint8_t *plive;
    PObject *obj;
    //platform_stream_lock(ps);
    msgpack_write(ps, MSGPCK_MAP, 4);


    msgpack_write(ps, MSGPCK_STRING, "th", 2);
    msgpack_write(ps, MSGPCK_UINT32, thid);

    msgpack_write(ps, MSGPCK_STRING, "i", 1);
    msgpack_write(ps, MSGPCK_ARRAY, 8);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hbase);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hend);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hedge);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hfreemem);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hblocks);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hfblocks);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)hlocked);
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)(100 - GC_FRAGMENTATION()));



    msgpack_write(ps, MSGPCK_STRING, "heap", 4);
    msgpack_write(ps, MSGPCK_ARRAY, (hblocks + hfblocks) * 3);


    plive = hbase;
    while (plive != hedge) {
        obj = (PObject *)plive;
        if (GCH_FLAG(obj) != GC_FREE) {
            //debug("HEAP: %s\n",typestrings[PHEADERTYPE(obj)]);
        }
        msgpack_write(ps, MSGPCK_UINT32, (uint32_t)obj);
        msgpack_write(ps, MSGPCK_UINT32, obj->header.gch.next);
        msgpack_write(ps, MSGPCK_UINT16, GCH_SIZE(obj));
        plive += GCH_SIZE(obj);
    }
    obj = (PObject *)hedge;
    msgpack_write(ps, MSGPCK_UINT32, (uint32_t)obj);
    msgpack_write(ps, MSGPCK_UINT32, obj->header.gch.next);
    msgpack_write(ps, MSGPCK_UINT16, 0);



    msgpack_write(ps, MSGPCK_STRING, "all", 3);
    msgpack_write(ps, MSGPCK_ARRAY, hblocks);
    plive = hbase;
    uint8_t cnt = 0;
    while (plive != hedge) {
        obj = (PObject *)plive;
        if (GCH_FLAG(obj) != GC_FREE) {
            pobj_print(ps, obj);
            //debug("ALL:%s\n",typestrings[PHEADERTYPE(obj)]);
        }
        plive += GCH_SIZE(obj);
    }

    //platform_stream_unlock(ps);
}
*/

void gc_trace() {

#if VM_DEBUG2
    uint8_t *plive;
    uint32_t flags;
    PObject *obj;
    PObject *next = 0;
    plive = hbase;
    debug( "\n#### TRACE\n");

    while (plive != hedge) {
        obj = (PObject *)plive;
        flags = GCH_FLAG(obj);
        if (!flags) {
            debug( "%s| F : %i @ %x :: %x\n", (next == obj) ? "|-->" : "|***", GCH_SIZE(obj), obj, GCH_NEXT(obj));
            next = GCH_NEXT(obj);
        } else if (flags == GC_USED) {
            debug( "%s| U : %i @ %x :: %s\n", (next) ? "|   " : "    ", GCH_SIZE(obj), obj, typestrings[PHEADERTYPE(obj)]);
        } else if (flags == GC_STAGED) {
            debug( "%s| S : %i @ %x :: %s -> %x <- %x\n", (next) ? "|   " : "    ", GCH_SIZE(obj), obj, typestrings[PHEADERTYPE(obj)], GCH_HEAP_NEXT(obj), GCH_HEAP_PREV(obj));
        } else {
            debug( "%s| M : %i @ %x :: %s\n", (next) ? "|   " : "    ", GCH_SIZE(obj), obj, typestrings[PHEADERTYPE(obj)]);
        }
        plive += GCH_SIZE(obj);
    }
    obj = (PObject *)hedge;
    debug( "--->| F : %i @ %x :: %x - %i\n", (uint32_t)(hend - hedge), obj, GCH_NEXT(obj), GCH_SIZE(obj));
    debug( "\n");

    for (flags = 0; flags < GC_HEAPS; flags++) {
        debug( "HEAP %i\n", flags);
        next = _gcheaps[flags];
        obj = next;
        if (obj) {
            do {
                debug( ". %x is %s of %i > %x < %x\n", obj, typestrings[PHEADERTYPE(obj)], GCH_SIZE(obj), GCH_HEAP_NEXT(obj), GCH_HEAP_PREV(obj));
                obj = GCH_HEAP_NEXT(obj);
            } while (next != obj);
        }
        debug( "---\n");

    }

    debug( "\n");

    debug( "GC KEEP\n");
    obj = _phead;
    while (obj) {
        debug( ". %x is %s of %i > %x :: %i\n", obj, typestrings[PHEADERTYPE(obj)], GCH_SIZE(obj), GCH_NEXT(obj), GCH_FLAG(obj));
        obj = GCH_NEXT(obj);
    }
    debug( "---\n");
    debug( "\n");


#endif

}