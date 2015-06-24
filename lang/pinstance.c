#include "lang.h"



PObject *pclass_new(int nparents, PObject **parents) {
    PClass *res = ALLOC_OBJ(PClass, PCLASS, 0, 0);
    res->dict = NULL;
    res->bases = ptuple_new(nparents, parents);

    while (nparents) {
        nparents--;
        debug("parent %i = %i\r\n", nparents, parents[nparents]);
    }
    return (PObject*)res;
}


err_t pclass_init(PClass *cls, PFrame *frm) {
    PCode *code = PCODE_MAKE(frm->code);
    PObject **locals = frm->storage + code->stacksize;
    MiniTable *mtable = (MiniTable *)(ViperCode(code)->bytecode + ViperCode(code)->nmstart);
    int sz = mtable->size;
    mtable++;
    cls->dict = pdict_new(sz);
    while (sz) {
        debug("put %i,%i\r\n", mtable->entry.name, mtable->entry.pos);
        pdict_put(cls->dict, PSMALLINT_NEW(mtable->entry.name), locals[mtable->entry.pos]);
        mtable++;
        sz--;
    }
    return ERR_OK;
}


int pclass_has_parent(PClass *oo, PClass *cls) {
    if (oo == cls) return 1;

    if (oo->bases) {
        int i;
        for (i = 0; i < PSEQUENCE_ELEMENTS(oo->bases); i++) {
            PClass *pcls = (PClass *)PTUPLE_ITEM(oo->bases, i);
            if (pclass_has_parent(pcls, cls))
                return 1;
        }
    }

    return 0;
}

PObject *pclass_get(PClass *cls, uint16_t name) {
    PObject *res = pdict_get(cls->dict, PSMALLINT_NEW(name));
    debug("::searching class %i for %i = %i and bases %i %i\r\n",cls,name,res,cls->bases,PSEQUENCE_ELEMENTS(cls->bases));
    if (!res) {
        PObject **buf = (PObject **) PSEQUENCE_BYTES(cls->bases);
        int els = PSEQUENCE_ELEMENTS(cls->bases);
        while (els) {
            debug("searching parent %i @ %i\r\n",els,*buf);
            res = pclass_get((PClass*)*buf, name);
            if (res)
                return res;
            buf++;
            els--;
        }
    }
    return res;
}

PObject *pinstance_new(PClass *cls) {
    PInstance *res = ALLOC_OBJ(PInstance, PINSTANCE, 0, 0);
    res->base = cls;
    res->dict = pdict_new(4);
    return (PObject*)res;
}

PObject *pinstance_get(PInstance *obj, uint16_t name) {
    PObject *res = pdict_get(obj->dict, PSMALLINT_NEW(name));
    debug("getting %i = %i\r\n", name, res);
    if (!res) {
        debug("searching in class %i\r\n", obj->base);
        res = pclass_get(obj->base, name);
        if (res) {
            if (PTYPE(res) == PFUNCTION) {
                /*generate a method*/
                debug("generate method from %i\r\n", res);
                res = (PObject*)pmethod_new((PCallable*)res, (PObject*)obj);
            }
            pdict_put(obj->dict, PSMALLINT_NEW(name), res);
        }
    }
    return res;
}


err_t pclass_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **res) {
    PClass *cls = (PClass *)o;
    if (b == P_MARKER) {
        /* load attr */
        *res = pclass_get(cls, name);
        if (!*res)
            return ERR_ATTRIBUTE_EXC;
    } else if (b) {
        /*store attr */
        pdict_put(cls->dict, PSMALLINT_NEW(name), b);
    } else {
        /*del attr*/
        return ERR_NOT_IMPLEMENTED_EXC;
    }
    return ERR_OK;
}

err_t pinstance_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **res) {
    PInstance *oo = (PInstance *)o;
    debug("pinstance_attr_fn: %i %i %i\r\n", o, name, b);
    if (b == P_MARKER) {
        /* load attr */
        *res = pinstance_get(oo, name);
        if (!*res)
            return ERR_ATTRIBUTE_EXC;
    } else if (b) {
        /*store attr */
        pdict_put(oo->dict, PSMALLINT_NEW(name), b);
    } else {
        /*del attr*/
        return ERR_NOT_IMPLEMENTED_EXC;
    }
    debug("pinstance_attr_fn: %i %i %i ret %i\r\n", o, name, b, *res);
    return ERR_OK;
}
