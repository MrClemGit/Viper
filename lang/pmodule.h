#ifndef __PMODULE__
#define __PMODULE__


#include "pobj.h"
#include "phash.h"
#include "pframe.h"



typedef struct _pmodule {
    PObjectHeader header;
    MiniTable *dict;
    union _modinfo {
        struct _glbcd {
            uint16_t nglobals;
            uint16_t code;
        } nums;
        PFrame *frame;
    } nfo;
    PObject *globals[];
} PModule;


PModule *pmodule_new(PFrame *frame);
err_t pmodule_attr_fn(PObject *o, uint16_t name, PObject *b, PObject **res);


#define PMODULE_SET_LOADED(m) (((PObject*)(m))->header.flags|=0x80)
#define PMODULE_IS_LOADED(m) (((PObject*)(m))->header.flags&(0x80))



#endif