#include "lang.h"



#ifdef VM_DEBUG
const char *const typestrings[] = {
    "SMALLINT", "INTEGER", "FLOAT", "BOOL", "STRING", "BYTES", "BYTEARRAY", "SHORTS", "SHORTARRAY", "LIST", "TUPLE", "RANGE",
    "FROZENSET", "SET", "DICT", "FUNCTION", "METHOD", "CLASS", "INSTANCE", "MODULE", "BUFFER", "SLICE", "ITERATOR", "FRAME", "CELL",
    "NONE", "EXCEPTION", "NATIVE",  "SYSOBJ", "DRIVER", "THREAD"
};
#endif

err_t pobj_compare(uint32_t op, PObject *a, PObject *b, PObject **res) {

    //debug("pobj_compare %i %i %i %i\r\n", a, b, PTYPE(a), PTYPE(b));
    if ((IS_PNUMBER(a) || IS_BOOL(a)) && (IS_PNUMBER(b) || IS_BOOL(b)))
        return pnumber_binary_op(op, a, b, res);

    if (IS_PSEQUENCE(a) && IS_PSEQUENCE(b))
        return psequence_compare(op, (PSequence *)a, (PSequence *)b, res);

    if (PHEADERTYPE(a) == PHEADERTYPE(b)) {
        /* TODO: switch on remaining types */
        /*
        if (IS_PSET(a)) {

        } else if (IS_PDICT(a)) {

        }*/
    }

    if (_BIN_OP(EQ) == op || _BIN_OP(NOT_EQ) == op) {
        *res = (_BIN_OP(EQ) == op) ? P_FALSE : P_TRUE;
        return ERR_OK;
    }
    return ERR_TYPE_EXC;
}

int pobj_is_true(PObject *a) {

    switch (PTYPE(a)) {
        case PSMALLINT:
            return a != P_ZERO;
        case PINTEGER:
            return ((PInteger *)a)->val != 0;
        case PFLOAT:
            return ((PFloat *)a)->val != FLOAT_ZERO;
        case PBOOL:
            return a == P_TRUE;
        case PSTRING:
        case PBYTES:
        case PBYTEARRAY:
        case PSHORTS:
        case PSHORTARRAY:
        case PLIST:
        case PTUPLE:
        case PRANGE:
            return ((PSequence *)a)->elements != 0;
        case PNONE:
            return 0;
        /*TODO: add types */
        default:
            return 1;
    }
}


int pobj_hash_byte_seq(PImmutableSequence *seq) {
    uint32_t hash = 5381;
    int32_t n;
    uint8_t *bsq = (uint8_t *)seq->seq;

    n = seq->elements - 1;
    /*while (n >= 4) {
        hash = ((hash << 5) + hash)^*(((uint32_t *)(seq->seq)) + n);
        n -= 4;
    }*/
    while (n >= 0) {
        hash = ((hash << 5) + hash) ^ (bsq[n]); /* hash * 33 + c */
        n--;
    }
    debug("computing hash for %i %i(%i)=%s\r\n",seq,hash,seq->elements,seq->seq);

    return (hash >> 1);
}

int pobj_hash_obj_seq(PObject **objs, int n) {
    uint32_t hash = 5381;
    int c;

    while (n > 0) {
        c = pobj_hash(objs[n - 1]);
        if (c < 0) return c;
        hash = ((hash << 5) + hash)^c; /* hash * 33 + c */
        n--;
    }
    return (hash >> 1);
}

int pobj_hash(PObject *a) {

    if (IS_TAGGED(a)) return ((int)a);
    if (!PCHECK_HASHABLE(a))
        return -1;

    switch (PTYPE(a)) {
        case PSMALLINT:
            return (int)a;
        case PINTEGER:
            return (int)((PInteger *)a)->val;
        case PFLOAT:
            return (int)((PFloat *)a)->val;
        /*
        case PCOMPLEX:
            return (int)((PComplex *)a)->re;
        */
        case PSTRING:
            return pobj_hash_byte_seq((PImmutableSequence *)a);
        case PTUPLE:
            return pobj_hash_obj_seq((PObject **)((PTuple *)a)->seq, ((PTuple *)a)->elements);
        case PRANGE:
            return ((PRange *)a)->start * 3 + ((PRange *)a)->stop * 5 + ((PRange *)a)->step * 7;
        case PBOOL:
            return PBOOL_VALUE(a);
    }
    return -1;
}

int pobj_equal(PObject *a, PObject *b) {
    PObject *res;
    //debug("pobj_equal %i %i\r\n", a, b);
    if (pobj_compare(_BIN_OP(EQ), a, b, &res) != ERR_OK)
        return 0;
    return (res == P_FALSE) ? 0 : 1;
}


PObject *pobj_getattr(PObject *obj, uint16_t name) {
    int tt = PTYPE(obj);
    if (tt == PINSTANCE || tt == PCLASS) {
        return (tt == PINSTANCE) ? pinstance_get((PInstance *)obj, name) : pclass_get((PClass *)obj, name);
    } else {
        PObject *res = get_native_mth(obj, name);
        return res;
    }
}


