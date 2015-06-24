#include "lang.h"

/* here we are: chained scatter tables */

#define HASH(a,n) (((int32_t)(HASHi(a)))%(n))
#define HASHi(a) (IS_PSMALLINT(a)  ? ((int)(a)):(pobj_hash(a)))
#define NTABLE_NONE 0
#define NTABLE_NOKEY NULL
#define NTABLE_NONEXT 0xffff


#define D_ENTRY(i) ((HashEntry*)(entrybuf+(i)*entrysize))
#define ENTRYBUFFER(f) (uint8_t*)(f->entry->buffer)
#define ENTRYSIZE(f) int entrysize=PHASH_ESIZE(f)

#define HAS_VAL(f) (PHEADERTYPE(f)==PDICT)


#ifdef VM_DEBUG
void phash_print(PHash *f) {
    ENTRYSIZE(f);
    info("%i::%i::%i:: %s\n", entrysize, f->elements, f->size, (HAS_VAL(f)) ? "DICT" : "SET");
    uint8_t *entrybuf = ENTRYBUFFER(f);
    int i;
    for (i = 0; i < f->size; i++) {
        HashEntry *entry = D_ENTRY(i);
        info("e %i: %x %x %i %i\n", i, entry->key, (HAS_VAL(f)) ? entry->value : NULL, entry->index, entry->next);
    }
}
#endif

void phash_init(PHash *f, int n) {
    //int tsize = n < 4 ? 4 : n;
    //f->entry = pbuffer_new(tsize, entrysize);
    ENTRYSIZE(f);
    f->size = n;
    f->elements = 0;
    HashEntry de;
    de.value = NULL;
    de.key = NTABLE_NOKEY;
    de.next = NTABLE_NONEXT;
    de.index = 0;
    uint8_t *db = ENTRYBUFFER(f);
    while (n > 0) {
        memcpy(db, &de, entrysize);
        n--;
        db += entrysize;
    }
    debug("Hash init for %x\n",f);
    phash_print(f);

}

PHash *phash_clone(PHash *src) {
    PHash *o;
    if (PHEADERTYPE(src) == PDICT) {
        o = (PHash *) pdict_new(src->size);
    } else {
        o = (PHash *) pset_new(PHEADERTYPE(src), src->size);
    }
    memcpy(PHASH_BYTES(o), PHASH_BYTES(src), PHASH_BYTES_SIZE(src));
    o->elements = src->elements;
    debug("Hash clone for %x\n",o);
    phash_print(o);
    return o;
}


void phash_rehash(PHash *f) {
    ENTRYSIZE(f);
    uint16_t tsize = f->size + 8;
    PBuffer *b_old = f->entry;
    PBuffer *b_new = pbuffer_new(tsize, entrysize);
    uint8_t *entrybuf = (uint8_t *)b_old->buffer;
    int i = f->elements - 1;
    //protect from gc
    gc_wait();
    f->entry = b_new;
    phash_init(f, tsize);
    HashEntry *eold_i, *eold_o;
    for (; i >= 0; i--) {
        eold_i = D_ENTRY(i);
        eold_o = D_ENTRY(eold_i->index);
        phash_put(f, eold_o->key, HAS_VAL(f) ? eold_o->value : NULL);
    }
    gc_signal();
}


int phash_put(PHash *f, PObject *key, PObject *val) {
    ENTRYSIZE(f);
    if (f->elements == f->size) {
        phash_rehash(f);
    }
    int probe = HASH(key, f->size);
    int place = NTABLE_NONEXT;
    int tail = NTABLE_NONEXT;
    uint8_t *entrybuf = ENTRYBUFFER(f);
    HashEntry *entry = D_ENTRY(probe);

    if (entry->key != NTABLE_NOKEY) {
        do {
            entry = D_ENTRY(probe);
            if (entry->key == key || pobj_equal(entry->key, key)) {
                //f->keys[probe]=key;
                if (HAS_VAL(f))
                    entry->value = val;
                return 1;
            }
            tail = probe;
        } while ((probe = entry->next) != NTABLE_NONEXT);
        do {
            place = (place + 1) % f->size;
            entry = D_ENTRY(place);
        } while (entry->key != NTABLE_NOKEY);
    } else {
        place = probe;
    }
    entry = D_ENTRY(place);
    entry->key = key;
    if (HAS_VAL(f))
        entry->value = val;
    entry->next = NTABLE_NONEXT;
    entry = D_ENTRY(f->elements);
    entry->index = place;
    if (tail != NTABLE_NONEXT) {
        entry = D_ENTRY(tail);
        entry->next = place;
    }
    f->elements++;
    return 0;
}


PObject *phash_get(PHash *f, PObject *key) {
    ENTRYSIZE(f);
    int probe = HASH(key, f->size);
    uint8_t *entrybuf = ENTRYBUFFER(f);
    HashEntry *entry = D_ENTRY(probe);

    debug("before get %x %x %i %i\n",entry->key,entry->value,entry->next,entry->index);
    phash_print(f);
    while (probe != NTABLE_NONEXT && key != entry->key && !pobj_equal(key, entry->key)) {
        probe = entry->next;
        entry = D_ENTRY(probe);
    }
    PObject *res = NULL;

    if (probe != NTABLE_NONEXT) {
        if (HAS_VAL(f))
            res = entry->value;
        else
            res = entry->key;
    }
    return res;
}



HashEntry *phash_getentry(PHash *f, int n) {
    ENTRYSIZE(f);
    HashEntry *res = NULL;
    uint8_t *entrybuf = ENTRYBUFFER(f);
    //debug("get_entry %i/%i: entry[%i].index=%i\r\n",n,f->elements,n,entry[n].index);
    if (n < f->elements) {
        HashEntry *entry = D_ENTRY(n);
        HashEntry *ontry = D_ENTRY(entry->index);
        res = ontry;
        //res = &(entry[entry[n].index]);
    }
    return res;
}



PObject *phash_del(PHash *f, PObject *key) {
    ENTRYSIZE(f);
    if (!f->elements)
        return NULL;

    int probe = HASH(key, f->size);
    uint8_t *entrybuf = ENTRYBUFFER(f);
    HashEntry *entry = D_ENTRY(probe);
    while (probe != NTABLE_NONEXT && key != entry->key && !pobj_equal(key, entry->key)) {
        probe = entry->next;
        entry = D_ENTRY(probe);
    }
    if (probe == NTABLE_NONEXT)
        return NULL;

    phash_print(f);
    debug("del @ %i\n", probe);
    PObject *res = (HAS_VAL(f)) ? (entry->value) : (entry->key);
    //let's search down the chain starting from probe
    int j;
    HashEntry *entry_j, *entry_k;
    while (1) {
        entry = D_ENTRY(probe);
        j = entry->next;
        while (j != NTABLE_NONEXT) {
            entry_j = D_ENTRY(j);
            int k = entry->next;
            int h = HASH(entry_j->key, f->size);
            int in_hashed_pos = 0;
            while (k != entry_j->next && !in_hashed_pos) {
                debug("i = %i, j = %i, k = %i, h = %i\n", probe, j, k, h);
                entry_k = D_ENTRY(k);
                if (k == h) {
                    //same hash
                    in_hashed_pos = 1;
                }
                k = entry_k->next;
                debug("k->%i | %i\n", k, in_hashed_pos);
            }
            if (!in_hashed_pos)
                break;
            j = entry_j->next;
            debug("j->%i\n", j);
        }
        if (j == NTABLE_NONEXT)
            break;
        debug("e[%i]=e[%i]\n", probe, j);
        entry->key = entry_j->key;
        if (HAS_VAL(f)) {
            entry->value = entry_j->value;
        }
        probe = j;
        debug("i->%i\n", probe);
    }
    entry = D_ENTRY(probe);
    entry->key =  NTABLE_NOKEY;
    entry->next = NTABLE_NONEXT;
    if (HAS_VAL(f))
        entry->value = NULL;

    //remove every probe from the next fields
    for (j = 0; j < f->elements; j++) {
        entry_j = D_ENTRY(j);
        if (entry_j->index == probe) {
            entry_k = D_ENTRY(f->elements - 1);
            entry_j->index = entry_k->index;
        }
        entry = D_ENTRY(entry_j->index);
        if (entry->next == probe)
            entry->next = NTABLE_NONEXT;
    }
    f->elements--;

    return res;
}




/* ========================================================================
    DICT METHODS
   ======================================================================== */



PDict *pdict_new(int size) {
    int tsize = size < 4 ? 4 : size;
    PDict *f = ALLOC_OBJ(PDict, PDICT, sizeof(DictEntry), 0);
    f->entry = pbuffer_new(tsize, sizeof(DictEntry));
    phash_init(f, tsize);
    return f;
}

err_t pdict_subscr_fn(PObject *o, PObject *i, PObject *b, PObject **res) {
    PHash *dict = (PHash *)o;
    if (b == P_MARKER) {
        /*load subscr:  dict[i]*/
        *res = pdict_get(dict, i);
        if (!*res)
            return ERR_INDEX_EXC;
    } else if (b) {
        /*store subscr: dict[i]=b */
        if (!PCHECK_HASHABLE(i))
            return ERR_TYPE_EXC;
        pdict_put(dict, i, b);
    } else {
        /*del subscr: dict[i]*/
        *res = pdict_del(dict, i);
        if (!*res)
            return ERR_KEY_EXC;
    }
    return ERR_OK;
}


err_t pdict_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res) {
    if (op == _BIN_OP(IN) || op == _BIN_OP(IN_NOT)) {
        *res = (phash_get((PHash *)a, b)) ?  ((op == _BIN_OP(IN)) ? P_TRUE : P_FALSE) : ( (op == _BIN_OP(IN)) ? P_FALSE : P_TRUE);
        return ERR_OK;
    }
    return ERR_TYPE_EXC;
}




NATIVE_FN(__dict_get) {
    NATIVE_UNWARN();
    PObject *val = pdict_get((PDict *)self, args[0]);
    if (!val)
        *res = args[1];
    else
        *res = val;
    return ERR_OK;
}
NATIVE_FN(__dict_keys) {
    NATIVE_UNWARN();
    PIterator *rr = piterator_new(self);
    if (!rr)
        return ERR_TYPE_EXC;
    PITERATOR_SET_TYPE(rr, PITERATOR_KEYS);
    *res = (PObject *)rr;
    return ERR_OK;
}
NATIVE_FN(__dict_items) {
    NATIVE_UNWARN();
    PIterator *rr = piterator_new(self);
    if (!rr)
        return ERR_TYPE_EXC;
    PITERATOR_SET_TYPE(rr, PITERATOR_ITEMS);
    *res = (PObject *)rr;
    return ERR_OK;
}
NATIVE_FN(__dict_values) {
    NATIVE_UNWARN();
    PIterator *rr = piterator_new(self);
    if (!rr)
        return ERR_TYPE_EXC;
    PITERATOR_SET_TYPE(rr, PITERATOR_VALUES);
    *res = (PObject *)rr;
    return ERR_OK;
}
NATIVE_FN(__dict_update) {
    NATIVE_UNWARN();
    if (PTYPE(args[0]) != PDICT)
        return ERR_NOT_IMPLEMENTED_EXC;
    PHash *dict = (PHash *)args[0];
    int i;
    for (i = 0; i < dict->elements; i++) {
        HashEntry *h = phash_getentry(dict, i);
        phash_put((PHash *)self, h->key, h->value);
    }
    *res = P_NONE;
    return ERR_OK;
}
NATIVE_FN(__dict_popitem) {
    NATIVE_UNWARN();
    PHash *dict = (PHash *)self;
    if (dict->elements == 0)
        return ERR_KEY_EXC;
    HashEntry *h = phash_getentry(dict, 0);
    phash_del(dict, h->key);
    PTuple *tpl = ptuple_new(2, NULL);
    PTUPLE_SET_ITEM(tpl, 0, h->key);
    PTUPLE_SET_ITEM(tpl, 1, h->value);
    *res = (PObject *)tpl;
    return ERR_OK;
}

NATIVE_FN(__dict_pop) {
    NATIVE_UNWARN();
    PHash *dict = (PHash *)self;
    PObject *key = args[0];
    PObject *def = (nargs == 2) ? args[1] : NULL;
    PObject *val = phash_del(dict, key);
    if (!val) {
        if (!def)
            return ERR_KEY_EXC;
        *res = def;
    } else {
        *res = val;
    }
    return ERR_OK;
}

NATIVE_FN(__hash_clear) {
    NATIVE_UNWARN();
    phash_init((PHash *)self, PHASH_SIZE(self));
    *res = P_NONE;
    return ERR_OK;
}
NATIVE_FN(__hash_copy) {
    NATIVE_UNWARN();
    *res = (PObject *)phash_clone((PHash *)self);
    return ERR_OK;
}


/* ========================================================================
    SET METHODS
   ======================================================================== */

PSet *pset_new(int type, int size) {
    int tsize = size < 4 ? 4 : size;
    PSet *f = ALLOC_OBJ(PSet, type, ((type == PSET) ? 0 : PFLAG_HASHABLE) | sizeof(SetEntry), 0);
    f->entry = pbuffer_new(tsize, sizeof(SetEntry));
    phash_init(f, tsize);
    return f;
}

typedef void( *__set_op_fn)(PObject *self, PObject *set, PObject *e, int *flag);

typedef struct __set_op_pnt {
    int nargs;
    PObject *self;
    PObject *set;
    PObject **args;
    __set_op_fn fn;
} SetOpHlp;

err_t __set_op_looper(SetOpHlp *op, int *flag) {
    int nargs = op->nargs;
    int i, j;
    for (i = 0; i < nargs; i++) {
        debug("__set_op_looper: %i/%i\n",i,nargs);
        PObject *arg = op->args[i];
        int tt = PTYPE(arg);
        if (IS_PSEQUENCE_TYPE(tt)) {
            for (j = 0; j < PSEQUENCE_ELEMENTS(arg); j++) {
                PObject *e;
                err_t err = psequence_getitem((PSequence *)arg, PSMALLINT_NEW(i), &e);
                if (err == ERR_OK) {
                    if (!PCHECK_HASHABLE(e))
                        return ERR_TYPE_EXC;
                    op->fn(op->self, op->set, e, flag);
                }
                else
                    return err;
            }
        } else if (IS_MAP_TYPE(tt)) {
            for (j = 0; j < PHASH_ELEMENTS(arg); j++) {
                HashEntry *e = phash_getentry((PHash *)arg, j);
                debug("__set_op_looper: map type %i/%i %x\n",j,PHASH_ELEMENTS(arg),e->key);
                if (!PCHECK_HASHABLE(e->key))
                    return ERR_TYPE_EXC;
                debug("calling %x(%x,%x,%x,%x)\n",op->fn,op->self, op->set, e->key, flag);
                op->fn(op->self, op->set, e->key, flag);
            }
        } else return ERR_TYPE_EXC;
    }
    return ERR_OK;
}


void __set_op_intersection(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    if (pset_get((PHash *)self, e))
        pset_put((PHash *)set, e);
}

void __set_op_union(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    pset_put((PHash *)set, e);
}

void __set_op_difference(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    pset_del((PHash *)set, e);
}

void __set_op_symdiff(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    if (pset_get((PHash *)self, e)) {
        pset_del((PHash *)set, e);
    } else {
        pset_put((PHash *)set, e);
    }
}

void __set_op_count_in(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    if (pset_get((PHash *)self, e))
        *flag = *flag + 1;
}

void __set_op_count_in_u(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    if (!pset_get((PHash *)set, e)) {
        pset_put((PHash *)set, e);
        if (pset_get((PHash *)self, e)) {
            *flag = *flag + 1;
        }
    }
}

void __set_op_update(PObject *self, PObject *set, PObject *e, int *flag) {
    (void)self;
    (void)set;
    (void)e;
    (void)flag;
    pset_put((PHash *)self, e);
}


NATIVE_FN(__set_difference) {
    NATIVE_UNWARN();
    PSet *set = phash_clone((PHash*)self);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_difference};
    err_t err = __set_op_looper(&op, NULL);
    *res = (PObject*)set;
    return err;
}


NATIVE_FN(__set_intersection) {
    NATIVE_UNWARN();
    PSet *set = pset_new(PSET, 0);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_intersection};
    err_t err = __set_op_looper(&op, NULL);
    *res = (PObject*)set;
    return err;
}
NATIVE_FN(__set_isdisjoint) {
    NATIVE_UNWARN();
    int cnt = 0;
    SetOpHlp op = {nargs, self, NULL, args, __set_op_count_in};
    err_t err = __set_op_looper(&op, &cnt);
    *res = (cnt == 0) ? P_TRUE : P_FALSE;
    return err;
}

NATIVE_FN(__set_issubset) {
    NATIVE_UNWARN();
    int cnt = 0;
    PSet *set = pset_new(PSET, 0);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_count_in_u};
    err_t err = __set_op_looper(&op, &cnt);
    *res = (cnt == PSET_ELEMENTS(self)) ? P_TRUE : P_FALSE;
    return err;
}
NATIVE_FN(__set_issuperset) {
    NATIVE_UNWARN();
    int cnt = 0;
    PSet *set = pset_new(PSET, 0);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_count_in_u};
    err_t err = __set_op_looper(&op, &cnt);
    *res = (cnt == PSET_ELEMENTS(set)) ? P_TRUE : P_FALSE;
    return err;
}
NATIVE_FN(__set_symmetric_difference) {
    NATIVE_UNWARN();
    PSet *set = phash_clone((PHash*)self);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_symdiff};
    err_t err = __set_op_looper(&op, NULL);
    *res = (PObject*)set;
    return err;
}
NATIVE_FN(__set_difference_update) {
    NATIVE_UNWARN();
    SetOpHlp op = {nargs, self, self, args, __set_op_difference};
    err_t err = __set_op_looper(&op, NULL);
    *res = (PObject*)self;
    return err;
}
NATIVE_FN(__set_symmetric_difference_update) {
    NATIVE_UNWARN();
    PSet *set = phash_clone((PHash*)self);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_symdiff};
    err_t err = __set_op_looper(&op, NULL);
    SYSLOCK();
    ((PHash *)self)->entry = ((PHash *)set)->entry;
    ((PHash *)self)->elements = ((PHash *)set)->elements;
    ((PHash *)self)->size = ((PHash *)set)->size;
    SYSUNLOCK();
    *res = (PObject*)self;
    return err;

}
NATIVE_FN(__set_intersection_update) {
    NATIVE_UNWARN();
    SetOpHlp op = {nargs, self, self, args, __set_op_intersection};
    err_t err = __set_op_looper(&op, NULL);
    *res = (PObject*)self;
    return err;
}


NATIVE_FN(__set_union) {
    NATIVE_UNWARN();
    PSet *set = phash_clone((PHash*)self);
    debug("__set_union: %x\n",__set_op_union);
    SetOpHlp op = {nargs, self, (PObject*)set, args, __set_op_union};
    err_t err = __set_op_looper(&op, NULL);
    *res = (PObject*)set;
    return err;
}

NATIVE_FN(__set_remove) {
    NATIVE_UNWARN();
    if (!PCHECK_HASHABLE(args[0]))
        return ERR_TYPE_EXC;
    if (!phash_del((PHash*)self, args[0]))
        return ERR_KEY_EXC;
    *res = P_NONE;
    return ERR_OK;
}

NATIVE_FN(__set_update) {
    NATIVE_UNWARN();
    SetOpHlp op = {nargs, self, NULL, args, __set_op_update};
    err_t err = __set_op_looper(&op, NULL);
    *res = P_NONE;
    return err;
}
NATIVE_FN(__set_pop) {
    NATIVE_UNWARN();
    if (!PSET_ELEMENTS(self))
        return ERR_KEY_EXC;
    HashEntry *h = pset_getentry((PHash*)self, 0);
    *res = phash_del((PHash*)self, h->key);
    return ERR_OK;
}
NATIVE_FN(__set_add) {
    NATIVE_UNWARN();
    if (!PCHECK_HASHABLE(args[0]))
        return ERR_TYPE_EXC;
    //debug("__set_add %x to %x|%i\n",args[0],self,PHASH_ELEMENTS(self));
    pset_put((PHash*)self, args[0]);
    phash_print((PHash*)self);
    *res = P_NONE;
    return ERR_OK;
}
NATIVE_FN(__set_discard) {
    NATIVE_UNWARN();
    if (!PCHECK_HASHABLE(args[0]))
        return ERR_OK;
    pset_del((PHash*)self, args[0]);
    *res = P_NONE;
    return ERR_OK;
}


err_t pset_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res) {
    err_t err;
    switch (op) {
        case _BIN_OP(IN):
        case _BIN_OP(IN_NOT):
            *res = (phash_get((PHash*)a, b)) ?  ((op == _BIN_OP(IN)) ? P_TRUE : P_FALSE) : ( (op == _BIN_OP(IN)) ? P_FALSE : P_TRUE);
            return ERR_OK;
        case _BIN_OP(LTE):
            err = NATIVE_FN_CALL(__set_issubset, 1, a, &b, res);
            return err;
        case _BIN_OP(LT):
            err = NATIVE_FN_CALL(__set_issubset, 1, a, &b, res);
            if (err == ERR_OK) {
                if (*res == P_TRUE && PCONTAINER_ELEMENTS(a) < PCONTAINER_ELEMENTS(b)) {
                    *res = P_TRUE;
                } else {
                    *res = P_FALSE;
                }
            }
            return err;
        case _BIN_OP(GTE):
            err = NATIVE_FN_CALL(__set_issuperset, 1, a, &b, res);
            return err;
        case _BIN_OP(GT):
            err = NATIVE_FN_CALL(__set_issuperset, 1, a, &b, res);
            if (err == ERR_OK) {
                if (*res == P_TRUE && PCONTAINER_ELEMENTS(a) > PCONTAINER_ELEMENTS(b)) {
                    *res = P_TRUE;
                } else {
                    *res = P_FALSE;
                }
            }
            return err;
        case _BIN_OP(BIT_OR):
            err = NATIVE_FN_CALL(__set_union, 1, a, &b, res);
            return err;
        case _BIN_OP(BIT_AND):
            err = NATIVE_FN_CALL(__set_intersection, 1, a, &b, res);
            return err;
        case _BIN_OP(SUB):
            err = NATIVE_FN_CALL(__set_difference, 1, a, &b, res);
            return err;
        case _BIN_OP(BIT_XOR):
            err = NATIVE_FN_CALL(__set_symmetric_difference, 1, a, &b, res);
            return err;
        case _BIN_OP(IBIT_OR):
            if (PHEADERTYPE(a) == PSET)
                err = NATIVE_FN_CALL(__set_update, 1, a, &b, res);
            else
                err = NATIVE_FN_CALL(__set_union, 1, a, &b, res);
            return err;
        case _BIN_OP(IBIT_AND):
            if (PHEADERTYPE(a) == PSET)
                err = NATIVE_FN_CALL(__set_intersection_update, 1, a, &b, res);
            else
                err = NATIVE_FN_CALL(__set_intersection, 1, a, &b, res);
            return err;
        case _BIN_OP(ISUB):
            if (PHEADERTYPE(a) == PSET)
                err = NATIVE_FN_CALL(__set_difference_update, 1, a, &b, res);
            else
                err = NATIVE_FN_CALL(__set_difference, 1, a, &b, res);
            return err;
        case _BIN_OP(IBIT_XOR):
            if (PHEADERTYPE(a) == PSET)
                err = NATIVE_FN_CALL(__set_symmetric_difference_update, 1, a, &b, res);
            else
                err = NATIVE_FN_CALL(__set_symmetric_difference, 1, a, &b, res);
            return err;
    }
    return ERR_TYPE_EXC;
}


/* ========================================================================
    MINITABLE
   ======================================================================== */

#define MINITABLE_NONEXT 0xff

int minitable_lookup(MiniTable *table, Name name) {
    register uint16_t size = table->size;
    register uint16_t probe = name % size;
    table++; /* move to first element */

    while (probe != MINITABLE_NONEXT) {
        if (name == table[probe].entry.name)
            return table[probe].entry.pos;
        probe = table[probe].entry.next;
    }
    return MINITABLE_NO_ENTRY;
}

