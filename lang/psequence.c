#include "lang.h"



/* ========================================================================
    SEQUENCE CREATION
   ======================================================================== */


PBuffer *pbuffer_new(uint16_t elements, uint8_t typesize) {
    //debug("pbuffer_new %i %i %i\r\n",typesize,GC_ALIGN_SIZE(typesize),elements);
    //typesize = GC_ALIGN_SIZE(typesize);
    PBuffer *res = ALLOC_OBJ(PBuffer, PBUFFER, typesize, typesize * elements);
    return res;
}



/* Not usable for PRANGE */
PSequence *psequence_new(uint8_t type, uint16_t elements) {
    uint8_t secobjsize =  IS_OBJ_PSEQUENCE_TYPE(type) ? 4 : (IS_BYTE_PSEQUENCE_TYPE(type) ? 1 : 2);
    debug( "allocating seq of type %i with objsize %i and %i elements\r\n", type, secobjsize, elements);
    if (IS_TYPE_SEQ_MUTABLE(type)) {
        PMutableSequence *res;
        res = ALLOC_OBJ(PMutableSequence, type, MAKE_SEQ_FLAGS(1, secobjsize), 0);
        res->elements = 0;
        res->seq = (elements) ? pbuffer_new(elements, secobjsize) : NULL;
        return (PSequence *)res;
    } else {
        PImmutableSequence *res;
        res = ALLOC_OBJ(PImmutableSequence, type, MAKE_SEQ_FLAGS(0, secobjsize) | PFLAG_HASHABLE, elements * secobjsize);
        res->elements = elements;
        return (PSequence *)res;
    }
}

PString *pstring_new(uint16_t len, uint8_t *buf) {
    PString *res = (PString *)psequence_new(PSTRING, len);
    if (len)
        memcpy(res->seq, buf, len);
    return res;
}

PBytes *pbytes_new(uint16_t len, uint8_t *buf) {
    PBytes *res = (PBytes *)psequence_new(PBYTES, len);
    if (len)
        memcpy(res->seq, buf, len);
    return res;
}
PBytes *pshorts_new(uint16_t len, uint16_t *buf) {
    PShorts *res = (PShorts *)psequence_new(PSHORTS, len);
    if (len)
        memcpy(res->seq, buf, len*2);
    return res;
}


PList *plist_new(uint16_t len, PObject **buf) {
    PList *res = (PList *)psequence_new(PLIST, len);
    res->elements = len;
    if (len && buf)
        memcpy(res->seq->buffer, buf, len * sizeof(PObject *));
    return res;
}

PTuple *ptuple_new(uint16_t len, PObject **buf) {
    PTuple *res = (PTuple *)psequence_new(PTUPLE, len);
    if (len && buf)
        memcpy(res->seq, buf, len * sizeof(PObject *));
    return res;
}


PRange *prange_new(INT_TYPE start, INT_TYPE stop, INT_TYPE step) {
    PRange *res = ALLOC_OBJ(PRange, PRANGE, PFLAG_HASHABLE | MAKE_SEQ_FLAGS(0, 1), 0);
    res-> start = start;
    res-> stop = stop;
    res->step = step == 0 ? 1 : step;
    INT_TYPE el = stop - start + step;
    el = ((step > 0) ? (el - 1) : (el + 1)) / step;
    res->elements =  (el < 0) ? 0 : el;
    debug( "new_range: (%i,%i,%i) with %i elements\r\n", start, stop, step, res->elements);
    return res;
}
PSlice *pslice_new(PObject *start, PObject *stop, PObject *step) {
    PSlice *res = ALLOC_OBJ(PSlice, PSLICE, PFLAG_HASHABLE, 0);
    res-> start = start;
    res-> stop = stop;
    res->step = step;
    return res;
}


/* ========================================================================
    SIMPLE ITERATOR
   ======================================================================== */


PIterator *piterator_new(PObject *seq) {
    int tt = PTYPE(seq);
    if ((tt >= PSTRING && tt <= PDICT) || (tt == PITERATOR)) {
        PIterator *res = ALLOC_OBJ(PIterator, PITERATOR, 0, 0);
        res->current = 0;
        res->temp = 0;
        res->iterable = seq;
        return res;
    }
    return NULL;
}

PObject *piterator_next(PIterator *p) {
    /* TODO: add support for non sequences */
    PObject *rr = NULL;
    int tt = PHEADERTYPE(p->iterable);

    if (tt == PITERATOR) {
        rr = (PObject *) piterator_next((PIterator *)p->iterable);
    } else if (IS_MAP_TYPE(tt)) {
        //debug("Iterator map\n");
        //phash_print(p->iterable);
        HashEntry *ee = NULL;
        uint16_t idx = ((PITERATOR_GET_TYPE(p) == PITERATOR_REVERSED)) ? (PMAP_ELEMENTS(p->iterable) - p->current - 1) :
                       (p->current);
        switch (PITERATOR_GET_TYPE(p)) {
            case PITERATOR_NORMAL:
            case PITERATOR_ENUMERATE:
            case PITERATOR_KEYS:
            case PITERATOR_REVERSED:
                ee = phash_getentry((PDict *)p->iterable, idx);
                rr = (ee) ? ee->key : NULL;
                break;
            case PITERATOR_VALUES:
                ee = pdict_getentry((PDict *)p->iterable, idx);
                rr = (ee) ? ee->value : NULL;
                break;
            case PITERATOR_ITEMS:
                ee = pdict_getentry((PDict *)p->iterable, idx);
                if (ee) {
                    PObject *dt[2] = {ee->key, ee->value};
                    rr = (PObject *)ptuple_new(2, dt);
                } else rr = NULL;
                break;
        }
        //debug("Iterator map got %x\n",rr);
    }  else {
        PSequence *seq = (PSequence *)(p->iterable);
        debug( "piterator_next seq %i / %i /%i\r\n", p->current, seq->elements,PTYPE(seq));
        if (p->current >= seq->elements) return NULL;

        if (tt == PRANGE) {
            PRange *r = (PRange *)seq;
            rr = NULL;
            if (p->current < r->elements) {
                INT_TYPE res = r->start + p->current * r->step;
                debug( "range iter: current %i, start %i, step %i, stop %i, curval %i\r\n", p->current, r->start, r->step, r->stop, res);
                rr = (PObject *) pinteger_new(res);
            }
        } else {
            uint16_t idx = ((PITERATOR_GET_TYPE(p) == PITERATOR_REVERSED)) ? (PSEQUENCE_ELEMENTS(seq) - p->current - 1) :
                           (p->current);
            uint16_t secobjsize = SEQ_OBJ_SIZE(seq);
            uint8_t *buf = PSEQUENCE_BYTES(seq);

            if (PHEADERTYPE(seq) == PSTRING) {
                PString *r = pstring_new(1, buf + idx);
                rr = (PObject *) r;
            } else {
                if (secobjsize == 1) {
                    rr = (PObject *) pinteger_new(buf[idx]);
                } else if (secobjsize == 2) {
                    rr = (PObject *) pinteger_new( ((uint16_t *)buf)[idx]);
                } else {
                    rr = (PObject *) ((PObject **)buf)[idx];
                }
            }
            debug("Iterator seq got %x\n",rr);
        }
        // if (SEQ_OBJ_SIZE(seq) <= 2) {
        //     uint8_t *buf = PSEQUENCE_BYTES(seq);
        //     uint16_t idx = (PITERATOR_GET_TYPE(p) == PITERATOR_REVERSED) ? (PSEQUENCE_ELEMENTS(seq) - p->current - 1) :
        //                    (p->current);
        //     if (PHEADERTYPE(seq) == PSTRING) {
        //         PString *r = pstring_new(1, buf + idx);
        //         rr = (PObject *) r;
        //     } else {
        //         if (SEQ_OBJ_SIZE(seq) == 1) {
        //             rr = (PObject *) pinteger_new(buf[idx]);
        //         } else {
        //             rr = (PObject *) pinteger_new( ((uint16_t *)buf)[idx]);
        //         }
        //     }
        // } else {
        //     PObject **buf = PSEQUENCE_OBJECTS(seq);
        //     uint16_t idx = ((PITERATOR_GET_TYPE(p) == PITERATOR_REVERSED)) ? (PSEQUENCE_ELEMENTS(seq) - p->current - 1) :
        //                    (p->current);
        //     debug( "ITERATOR on %i at %i sz=4 size with mutable = %i %i\n", seq, buf, IS_SEQ_MUTABLE(seq), buf[p->current]);
        //     rr = buf[idx];
        // }
    }
    p->current++;
    if (rr && PITERATOR_GET_TYPE(p) == PITERATOR_ENUMERATE) {
        PObject *ii = (PObject *) pinteger_new(p->temp + p->current - 1);
        PObject *dt[2] = {ii, rr};
        PTuple *rt = ptuple_new(2, dt);
        return (PObject *)rt;
    }

    return rr;

}


/* ========================================================================
    SEQUENCE OPS
   ======================================================================== */


/* PRANGE can't be multiplied */
err_t psequence_mul(PObject *n, PSequence *seq, PSequence **res, uint8_t inplace) {
    if (!IS_INTEGER(n) || PHEADERTYPE(seq) == PRANGE)
        return ERR_TYPE_EXC;

    INT_TYPE val = INTEGER_VALUE(n);
    uint32_t size_copied = 0;
    uint8_t secobjsize = SEQ_OBJ_SIZE(seq);
    uint32_t seqsize = seq->elements * secobjsize;
    uint8_t *buf;
    uint8_t *src;

    if (val < 0) val = 0;

    if (IS_SEQ_MUTABLE(seq)) {
        PMutableSequence *aa = (PMutableSequence *) seq;
        PMutableSequence *rr = (inplace) ? \
                               psequence_grow(aa, val * aa->elements) : \
                               (PMutableSequence *) psequence_new(PHEADERTYPE(seq), val * aa->elements);
        buf = (inplace) ? (_PMS_BYTES(rr) + (seqsize)) : (_PMS_BYTES(rr));
        src = _PMS_BYTES(aa);
        if (inplace) val--;
        rr->elements = val*aa->elements;
        *res = (PSequence *) rr;
    } else {
        PImmutableSequence *aa = (PImmutableSequence *) seq;
        PImmutableSequence *rr = (PImmutableSequence *) psequence_new(PHEADERTYPE(seq), val * aa->elements);
        buf = _PIS_BYTES(rr);
        src = _PIS_BYTES(aa);
        *res = (PSequence *) rr;
    }
    while (val > 0) {
        memcpy(buf + size_copied, src, seqsize);
        size_copied += seqsize;
        val--;
    }
    return ERR_OK;
}

/* PRANGE can't be concat'd */
err_t psequence_concat(PSequence *a, PSequence *b, PSequence **res, uint8_t inplace) {
    if ((PHEADERTYPE(a) == PRANGE))
        return ERR_TYPE_EXC;

    uint8_t secobjsize = SEQ_OBJ_SIZE(a);
    if (secobjsize != SEQ_OBJ_SIZE(b))
        return ERR_TYPE_EXC;

    int ssize = a->elements + b->elements;
    debug( "concat %i to %i (inplace %i) from %i,%i to %i\n", a, b, inplace, a->elements, b->elements, ssize);
    if (IS_SEQ_MUTABLE(a)) {
        PMutableSequence *aa = (PMutableSequence *)a;
        PMutableSequence *bb = (PMutableSequence *)b;
        PMutableSequence *rr = (inplace) ? \
                               psequence_grow(aa, ssize) : \
                               (PMutableSequence *)psequence_new(PHEADERTYPE(a), ssize);
        if (!inplace)
            memcpy(_PMS_BYTES(rr), _PMS_BYTES(aa), aa->elements * secobjsize);

        memcpy(_PMS_BYTES(rr) + aa->elements * secobjsize, _PMS_BYTES(bb), bb->elements * secobjsize);
        rr->elements = ssize;
        *res = (PSequence *)rr;
    } else {
        PImmutableSequence *aa = (PImmutableSequence *)a;
        PImmutableSequence *bb = (PImmutableSequence *)b;
        PImmutableSequence *rr = (PImmutableSequence *) psequence_new(PHEADERTYPE(a), ssize);
        memcpy(_PIS_BYTES(rr), _PIS_BYTES(aa), secobjsize * aa->elements);
        memcpy(_PIS_BYTES(rr) + (secobjsize * aa->elements), _PIS_BYTES(bb), secobjsize * bb->elements);
        *res = (PSequence *) rr;
    }
    return ERR_OK;
}

/* on mutable seq */
PMutableSequence *psequence_grow(PMutableSequence *seq, uint16_t size) {
    uint8_t secobjsize = SEQ_OBJ_SIZE(seq);
    PBuffer *oldseq = seq->seq;
    PBuffer *newseq;

    debug( "growing %i from %i to %i\n", seq, PSEQUENCE_SIZE(seq), size);
    if (size <= PSEQUENCE_SIZE(seq))
        return seq;

    debug( "allocating new buffer of size %i\n", size);
    newseq = pbuffer_new(size, secobjsize);
    debug( "copying data\n");
    memcpy(newseq->buffer, oldseq->buffer, seq->elements * secobjsize);
    seq->seq = newseq;
    return seq;
}



err_t psequence_compare(uint32_t opcode, PSequence *a, PSequence *b, PObject **res) {
    uint8_t secobjsizea = SEQ_OBJ_SIZE(a);
    uint8_t secobjsizeb = SEQ_OBJ_SIZE(b);
    int tmp = 1;


    if (PHEADERTYPE(a) != PHEADERTYPE(b)) {
        if (secobjsizea == secobjsizeb) {
            if ((PHEADERTYPE(a) == PSTRING || PHEADERTYPE(b) == PSTRING))
                return ERR_TYPE_EXC;
            } else return ERR_TYPE_EXC;
    }

    debug( "pseq_compare: %i vs %i\r\n", a->elements, b->elements);
    if ((a->elements != b->elements) && (opcode == _BIN_OP(EQ) || opcode == _BIN_OP(NOT_EQ))) {
        *res = (opcode == _BIN_OP(EQ)) ? P_FALSE : P_TRUE;
        return ERR_OK;
    }
    if (PHEADERTYPE(a) == PRANGE) {
        PRange *aa = (PRange *)a;
        PRange *bb = (PRange *)b;
        switch (opcode) {
            case _BIN_OP(EQ): tmp = (aa->elements == bb->elements && aa->start == bb->start && aa->step == bb->step); break;
            case _BIN_OP(NOT_EQ): tmp = !(aa->elements == bb->elements && aa->start == bb->start && aa->step == bb->step); break;
            case _BIN_OP(LT): tmp = (aa->start < bb->start && aa->elements < bb->elements); break;
            case _BIN_OP(LTE): tmp = (aa->start <= bb->start && aa->elements <= bb->elements); break;
            case _BIN_OP(GT): tmp = (aa->start > bb->start && aa->elements > bb->elements); break;
            case _BIN_OP(GTE): tmp = (aa->start >= bb->start && aa->elements >= bb->elements); break;
        }
        *res = (tmp) ? P_TRUE : P_FALSE;
        return ERR_OK;
    }

    uint16_t elems = (a->elements > b->elements) ? b->elements : a->elements;
    if (secobjsizea <= 2) {
        /* comparing strings or bytes/array or short/shortarray */
        uint8_t *abuf = PSEQUENCE_BYTES(a);
        uint8_t *bbuf = PSEQUENCE_BYTES(b);

        tmp = memcmp(abuf, bbuf, elems);
        debug( "pseq_compare: %i %i %i %i %i [%s],[%s]\r\n", tmp, a->elements, b->elements, elems, opcode, abuf, bbuf);
        switch (opcode) {
            case _BIN_OP(EQ): tmp = (tmp == 0) && (a->elements == b->elements); break;
            case _BIN_OP(NOT_EQ): tmp = (tmp != 0) || (a->elements != b->elements); break;
            case _BIN_OP(LT): tmp = (tmp < 0) && (a->elements < b->elements); break;
            case _BIN_OP(LTE): tmp = (tmp <= 0) && (a->elements <= b->elements); break;
            case _BIN_OP(GT): tmp = (tmp > 0) && (a->elements > b->elements); break;
            case _BIN_OP(GTE): tmp = (tmp >= 0) && (a->elements >= b->elements); break;
        }
    } else {
        PObject **abuf, **bbuf, *rrr;
        int i;
        err_t err;
        abuf = PSEQUENCE_OBJECTS(a);
        bbuf = PSEQUENCE_OBJECTS(b);

        for (i = 0; i < elems && tmp; i++) {
            err = pobj_compare(opcode, abuf[i], bbuf[i], &rrr);
            if (err != ERR_OK) return err;
            tmp &= (rrr == P_TRUE);
        }
        if (i == elems && tmp) {
            switch (opcode) {
                case _BIN_OP(EQ): tmp &= (a->elements == b->elements); break;
                case _BIN_OP(NOT_EQ): tmp &= (a->elements != b->elements); break;
                case _BIN_OP(LT): tmp &= (a->elements < b->elements); break;
                case _BIN_OP(LTE): tmp &= (a->elements <= b->elements); break;
                case _BIN_OP(GT): tmp &= (a->elements > b->elements); break;
                case _BIN_OP(GTE): tmp &= (a->elements >= b->elements); break;
            }
        }
    }
    *res = (tmp) ? P_TRUE : P_FALSE;
    return ERR_OK;

}


int psequence_contains(PSequence *a, PObject *b) {
    uint8_t secobjsize = SEQ_OBJ_SIZE(a);

    if (PHEADERTYPE(a) == PRANGE) {
        PRange *aa = (PRange *)a;
        if (!IS_INTEGER(b)) return 0;
        INT_TYPE val = INTEGER_VALUE(a);
        return (((val - aa->start) % aa->step == 0) && (val >= aa->start) && (val < aa->stop));
    }

    if (secobjsize == 1) {
        /* Strings and bytes */
        uint8_t *buf = PSEQUENCE_BYTES(a);
        int i, j, cnt = 0;
        if (IS_PSEQUENCE(b)) {
            /* searching subsequence b inside seq */
            PSequence *bb = ((PSequence *)b);
            if (SEQ_OBJ_SIZE(b) > 1) return 0;
            if (bb->elements > a->elements) return 0;
            uint8_t *bbuf = PSEQUENCE_BYTES(b);
            /*TODO: optimize search */
            for (i = 0; i < a->elements - bb->elements; i++) {
                cnt = 0;
                for (j = 0; j < bb->elements; j++)
                    if (buf[i + j] == bbuf[j]) cnt++;
                    else break;
                if (cnt == bb->elements) return 1;
            }
            return 0;
        } else {
            /* searching integer inside seq */
            if (!IS_INTEGER(b)) return 0;
            int32_t val = INTEGER_VALUE(b);
            if (val < 0 || val > 0xff) return 0;
            for (i = 0; i < a->elements; i++)
                if (val == buf[i]) return 1;
            return 0;
        }
    } else if (secobjsize == 2) {
        if (!IS_INTEGER(b)) return 0;
        int i;
        uint16_t *buf = (uint16_t *)PSEQUENCE_BYTES(a);
        int32_t val = INTEGER_VALUE(b);
        if (val < 0 || val > 0xffff) return 0;
        for (i = 0; i < a->elements; i++)
            if (val == buf[i]) return 1;
        return 0;
    } else {
        /* Object Containers */
        PObject **abuf = PSEQUENCE_OBJECTS(a);
        PObject *res = 0;
        int i;
        for (i = 0; i < a->elements; i++) {
            pobj_compare(_BIN_OP(EQ), abuf[i], b, &res);
            if (res == P_TRUE) return 1;
        }
    }
    return 0;

}


err_t psequence_setitem(PSequence *a, PObject *pos, PObject *b) {
    if (!IS_SEQ_MUTABLE(a))
        return ERR_TYPE_EXC;

    if (!IS_INTEGER(pos))
        return ERR_TYPE_EXC;

    INT_TYPE seqp = INTEGER_VALUE(pos);

    PSEQUENCE_INDEX_NORMALIZE(a, seqp);
    PSEQUENCE_INDEX_CHECK_BOUNDS(a, seqp);

    if (SEQ_OBJ_SIZE(a) <= 2) {
        /* bytearray / shortarray*/
        if (!IS_INTEGER(b))
            return ERR_TYPE_EXC;
        int32_t val = INTEGER_VALUE(b);
        if (SEQ_OBJ_SIZE(a) == 1) {
            if (val < 0 || val > 0xff)
                return ERR_VALUE_EXC;
            _PMS_SET_BYTE(a, seqp, val);
        } else {
            if (val < 0 || val > 0xffff)
                return ERR_VALUE_EXC;
            _PMS_SET_SHORT(a, seqp, val);
        }
        //((PMutableSequence *)a)->seq->buffer[seqp] = (uint8_t)val;
    } else {
        /* list */
        _PMS_SET_OBJECT(a, seqp, b);
        //((PObject **)(((PMutableSequence *)a)->seq->buffer))[seqp] = b;
    }
    return ERR_OK;
}

err_t psequence_getitem(PSequence *a, PObject *pos, PObject **res) {
    if (!IS_INTEGER(pos))
        return ERR_TYPE_EXC;

    INT_TYPE seqp = INTEGER_VALUE(pos);

    PSEQUENCE_INDEX_NORMALIZE(a, seqp);
    PSEQUENCE_INDEX_CHECK_BOUNDS(a, seqp);


    if (PHEADERTYPE(a) == PRANGE) {
        *res = (PObject *) pinteger_new(((PRange *)a)->start + ((PRange *)a)->step * a->elements);
    } else if (SEQ_OBJ_SIZE(a) == 1) {
        /* bytearray, bytes, string */
        uint8_t *abuf = PSEQUENCE_BYTES(a);
        *res = ((PHEADERTYPE(a) == PSTRING) ? \
                ((PObject *)pstring_new(1, abuf + seqp)) : \
                (PSMALLINT_NEW(abuf[seqp])));
    } else if (SEQ_OBJ_SIZE(a) == 2) {
        /* shortarray, shorts */
        uint16_t *abuf = (uint16_t *)PSEQUENCE_BYTES(a);
        *res =  PSMALLINT_NEW(abuf[seqp]);
    } else {
        /* list, tuple*/
        PObject **abuf = PSEQUENCE_OBJECTS(a);
        *res = abuf[seqp];
    }

    debug( "pseq_getitem %i[%i/%i]=%i\r\n", a, pos, seqp, res);

    return ERR_OK;
}


err_t psequence_getslice(PSequence *a, PSlice *s, PObject **res) {
    INT_TYPE start;
    INT_TYPE stop;
    INT_TYPE step;
    INT_TYPE ssize;

    PSLICE_PREPARE_INDEXES(a, s, start, stop, step);
    debug( "get slice of %i with %i:%i:%i\n", a, start, stop, step);
    PSLICE_INDEX_NORMALIZE(a, start, stop, step, ssize);
    debug( "    normalized to %i:%i:%i with size %i\n", start, stop, step, ssize);

    if (PHEADERTYPE(a) == PRANGE) {
        *res = (PObject *)prange_new( ((PRange *)a)->start + ((PRange *)a)->step * start,
                                      ((PRange *)a)->start + ((PRange *)a)->step * stop, step);
        return ERR_OK;
    }

    *res = (PObject *) psequence_new(PHEADERTYPE(a), ssize);
    ((PSequence *)(*res))->elements = ssize;

    if (SEQ_OBJ_SIZE(a) == 1) {
        /* bytearray, bytes, string */
        uint8_t *abuf = PSEQUENCE_BYTES(a);
        uint8_t *bbuf = PSEQUENCE_BYTES(*res);
        if (step == 1) {
            memcpy(bbuf, abuf + start, ssize);
        } else {
            while (ssize-- > 0) {
                *bbuf++ = abuf[start];
                start += step;
            }
        }
    } else if (SEQ_OBJ_SIZE(a) == 2) {
        uint16_t *abuf = (uint16_t *)PSEQUENCE_BYTES(a);
        uint16_t *bbuf = (uint16_t *)PSEQUENCE_BYTES(*res);
        if (step == 1) {
            memcpy(bbuf, abuf + start, ssize*2);
        } else {
            while (ssize-- > 0) {
                *bbuf++ = abuf[start];
                start += step;
            }
        }
    } else {
        /* list, tuple*/
        PObject **abuf = PSEQUENCE_OBJECTS(a);
        PObject **bbuf = PSEQUENCE_OBJECTS(*res);
        if (step == 1) {
            memcpy(bbuf, abuf + start, ssize * sizeof(PObject *));
        } else {
            while (ssize-- > 0) {
                *bbuf++ = abuf[start];
                start += step;
            }
        }
    }
    return ERR_OK;

}


err_t psequence_setslice(PSequence *a, PSlice *s, PObject *itr) {
    int32_t start;
    int32_t stop;
    int32_t step;
    int32_t ssize;
    uint8_t secobjsize = SEQ_OBJ_SIZE(a);

    if (!IS_SEQ_MUTABLE(a))
        return ERR_TYPE_EXC;

    /*TODO: extend setslice to iterables  like a[x:y:z] = set(...) */
    /* for now, restrict to sequences */

    if (!IS_PSEQUENCE(itr))
        return ERR_NOT_IMPLEMENTED_EXC;


    PSLICE_PREPARE_INDEXES(a, s, start, stop, step);
    debug( "set slice of %i with %i:%i:%i\n", a, start, stop, step);
    PSLICE_INDEX_NORMALIZE(a, start, stop, step, ssize);
    debug( "    normalized to %i:%i:%i with size %i\n", start, stop, step, ssize);

    PMutableSequence *aa = (PMutableSequence *)a;
    PSequence *bb = (PSequence *)itr;
    PBuffer *tpb;

    /* check slice vs iterable */
    if (step != 1 && ssize != bb->elements)
        return ERR_VALUE_EXC;

    /* adjust size *//* TODO: optimize, don't always allocate if space i enough */
    if (PSEQUENCE_SIZE(aa) < aa->elements + bb->elements - (uint32_t)ssize)
        tpb = pbuffer_new(aa->elements + bb->elements - ssize, secobjsize);
    else
        tpb = pbuffer_new(aa->elements + bb->elements - ssize, secobjsize);


    debug( "setting slice in %i(%i) with slice %i:%i:%i:%i to %i(%i)\n", a, a->elements, start, stop, step, ssize, tpb,
          aa->elements + bb->elements - ssize);
    if (secobjsize == 1) {
        /* bytearray, bytes, string */
        uint8_t *abuf = PSEQUENCE_BYTES(aa);
        uint8_t *bbuf = PSEQUENCE_BYTES(bb);
        uint8_t *rbuf = (uint8_t *)tpb->buffer;

        if (step == 1) {
            memcpy(rbuf, abuf, start * secobjsize);
            memcpy(rbuf + start, bbuf, bb->elements * secobjsize);
            memcpy(rbuf + start + bb->elements, abuf + stop, (aa->elements - stop)*secobjsize);
        } else {
            if (step > 0) {
                int cnt = 0;
                memcpy(rbuf, abuf, start * secobjsize);
                abuf += start;
                rbuf += start;
                while (ssize-- > 0) {
                    *rbuf++ = (cnt % (step) == 0) ? *bbuf++ : *abuf;
                    abuf++;
                    cnt++;
                }
                memcpy(rbuf, abuf, (aa->elements - (start + cnt))*secobjsize);
            } else {
                int cnt = -step;
                memcpy(rbuf, abuf, (stop + 1) * secobjsize);
                rbuf += (stop + 1);
                abuf += start;
                while (ssize-- > 0) {
                    *rbuf++ = (cnt % (-step) == 0) ? *bbuf++ : *abuf;
                    abuf--;
                    cnt++;
                }
                memcpy(rbuf, abuf + cnt, (aa->elements - start)*secobjsize);
            }
        }
    } else if (secobjsize == 2) {
        /* shortarray, shorts */
        uint16_t *abuf = (uint16_t*)PSEQUENCE_BYTES(aa);
        uint16_t *bbuf = (uint16_t*)PSEQUENCE_BYTES(bb);
        uint16_t *rbuf = (uint16_t *)tpb->buffer;

        if (step == 1) {
            memcpy(rbuf, abuf, start * secobjsize);
            memcpy(rbuf + start, bbuf, bb->elements * secobjsize);
            memcpy(rbuf + start + bb->elements, abuf + stop, (aa->elements - stop)*secobjsize);
        } else {
            if (step > 0) {
                int cnt = 0;
                memcpy(rbuf, abuf, start * secobjsize);
                abuf += start;
                rbuf += start;
                while (ssize-- > 0) {
                    *rbuf++ = (cnt % (step) == 0) ? *bbuf++ : *abuf;
                    abuf++;
                    cnt++;
                }
                memcpy(rbuf, abuf, (aa->elements - (start + cnt))*secobjsize);
            } else {
                int cnt = -step;
                memcpy(rbuf, abuf, (stop + 1) * secobjsize);
                rbuf += (stop + 1);
                abuf += start;
                while (ssize-- > 0) {
                    *rbuf++ = (cnt % (-step) == 0) ? *bbuf++ : *abuf;
                    abuf--;
                    cnt++;
                }
                memcpy(rbuf, abuf + cnt, (aa->elements - start)*secobjsize);
            }
        }
    }else {
        /* list, tuple*/
        PObject **abuf = PSEQUENCE_OBJECTS(aa);
        PObject **bbuf = PSEQUENCE_OBJECTS(bb);
        PObject **rbuf = (PObject **)(tpb->buffer);
        debug( "SETTING SLICE:\n");
        debug( "a: 0---------------------%i (%i)\n", aa->elements - 1, aa->elements);
        debug( "       %i:%i:%i:(%i)\n", start, stop, step, ssize);
        debug( "b: 0---------------------%i (%i)\n", bb->elements - 1, bb->elements);
        if (step == 1) {
            debug( "o   copying from 0 to %i (%i)\n", start, start);
            memcpy(rbuf, abuf, start * secobjsize);
            debug( "oo  copying from %i to %i (%i)\n", start, start + bb->elements, bb->elements);
            memcpy(rbuf + start, bbuf, bb->elements * secobjsize);
            debug( "ooo copying from %i to %i (%i)\n", start + bb->elements, start + bb->elements + (aa->elements - stop),
                  (aa->elements - stop));
            memcpy(rbuf + start + bb->elements, abuf + stop, (aa->elements - stop)*secobjsize);
        } else {
            if (step > 0) {
                int cnt = 0;
                memcpy(rbuf, abuf, start * secobjsize);
                abuf += start;
                rbuf += start;
                while (ssize-- > 0) {
                    *rbuf++ = (cnt % (step) == 0) ? *bbuf++ : *abuf;
                    abuf++;
                    cnt++;
                }
                memcpy(rbuf, abuf, (aa->elements - (start + cnt))*secobjsize);
            } else {
                int cnt = -step;
                memcpy(rbuf, abuf, (stop + 1) * secobjsize);
                rbuf += (stop + 1);
                abuf += start;
                while (ssize-- > 0) {
                    *rbuf++ = (cnt % (-step) == 0) ? *bbuf++ : *abuf;
                    abuf--;
                    cnt++;
                }
                memcpy(rbuf, abuf + cnt, (aa->elements - start)*secobjsize);
            }
        }
    }
    aa->seq = tpb;
    aa->elements = aa->elements + bb->elements - ssize;
    return ERR_OK;
}


err_t plist_append(PObject *l, PObject *x) {
    PList *list = (PList *)l;
    if (list->elements >= PSEQUENCE_SIZE(list)) {
        psequence_grow(list, list->elements + 8);
    }
    ((PObject **)list->seq->buffer)[list->elements++] = x;
    return ERR_OK;
}


err_t psequence_binary_op(uint32_t op, PObject *a, PObject *b, PObject **res) {
    /* a is a Psequence */
    if (op == _BIN_OP(MUL) || op == _BIN_OP(IMUL))
        return psequence_mul(b, (PSequence *)a, (PSequence **)res, op == _BIN_OP(IMUL));
    else if ((op == _BIN_OP(ADD) || op == _BIN_OP(IADD)) && IS_PSEQUENCE(b))
        return psequence_concat((PSequence *)a, (PSequence *)b, (PSequence **) res, op == _BIN_OP(IADD));
    else if (op == _BIN_OP(IN) || op == _BIN_OP(IN_NOT)) {
        *res = psequence_contains((PSequence *) a, b) ? P_TRUE : P_FALSE;
        return ERR_OK;
    } else if (!IS_PSEQUENCE(b)) {
        return ERR_TYPE_EXC;
    }
    return psequence_compare(op, (PSequence *)a, (PSequence *) b, res);
}

err_t psequence_subscr(PObject *a, PObject *idx, PObject *b, PObject **res) {
    if (b == P_MARKER) {
        /* it's a load_subscr */
        return (IS_PSLICE(idx)) ? psequence_getslice((PSequence *)a, (PSlice *)idx, res) : psequence_getitem((PSequence *)a,
                idx, res);
    } else {
        if (b == NULL) {
            /*it's a del_subscr */
            //return (IS_PSLICE(idx)) ? psequence_delslice(a, idx, res) : psequence_delitem(a, idx, res);
            return ERR_NOT_IMPLEMENTED_EXC;
        } else {
            /*it's a store_subscr */
            *res = a;
            return (IS_PSLICE(idx)) ? psequence_setslice((PSequence *)a, (PSlice *)idx, b) : psequence_setitem((PSequence *)a, idx,
                    b);
        }
    }
}
