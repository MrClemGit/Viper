#ifndef __PINSTANCES__
#define __PINSTANCES__

#include "pobj.h"
#include "phash.h"
#include "psequence.h"


typedef struct _pattributable {
    PObjectHeader header;
    PDict *dict;
} PAttributable;

typedef struct _pclass {
    PObjectHeader header;
    PDict *dict;
    PTuple *bases;
} PClass;


typedef struct _pinstance {
    PObjectHeader header;
    PDict *dict;
    PClass *base;
} PInstance;



err_t pclass_init(PClass *cls, PFrame *frm);
PObject *pclass_new(int nparents, PObject **parents);
PObject *pinstance_new(PClass *cls);
PObject *pclass_get(PClass *cls, uint16_t name);
PObject *pinstance_get(PInstance *obj, uint16_t name);


int pclass_has_parent(PClass *oo, PClass *cls);

err_t pclass_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **);
err_t pinstance_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **);

#endif



