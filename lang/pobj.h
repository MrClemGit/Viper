#ifndef __POBJ__
#define __POBJ__

#include "langdefs.h"
#include "ptypes.h"


typedef struct _p_obj PObject;

typedef union _gcheader {
    uint32_t flags;
    PObject *ref;
    PObject *next;
    struct _gchext {
        int16_t hprev;
        int16_t hnext;
    } ext; 
} GCHeader;

typedef struct _p_obj_header {
    GCHeader gch;
    uint16_t size;
    uint8_t type;
    uint8_t flags;
} PObjectHeader;

/* PObjectHeader flags                          */
/*    0    1    2    3    4    5    6    7      */
/*    |--------------|    |    |         |      */
/*   sequence            mut hashable    active,*/
/*    flags                              loaded */

#define PFLAG_OSIZE(o) (((PObject*)(o))->header.flags&0x1f)
#define PFLAG_HASHABLE  0x20
#define PFLAG_HAS_CLASS (0x1<<6)
#define PCHECK_HASHABLE(o) ((IS_TAGGED(o))||(((PObject*)(o))->header.flags&PFLAG_HASHABLE))

typedef struct _p_obj {
    PObjectHeader header;
} PObject;


#define PHEADERTYPE(o) (((PObject*)o)->header.type)
#define IS_TAGGED(o) (((uint32_t)(o))&(GC_ALIGNMENT-1))
//#define TYPE_FROM_TAGGED(o) ((((uint32_t)(o))&0x000000fe)>>2)
#define PTYPE(o) (   (IS_TAGGED(o)) ? (IS_PSMALLINT(o)  ? (PSMALLINT):(TYPE_FROM_TAGGED(o))):(PHEADERTYPE(o)) )
#define TAGTYPE(o) (IS_PSMALLINT(o) ? PSMALLINT:TYPE_FROM_TAGGED(o))


err_t pobj_compare(uint32_t op, PObject *a, PObject *b, PObject **res);
int pobj_is_true(PObject *a);
int pobj_hash(PObject *a);
int pobj_equal(PObject *a, PObject *b);

PObject *pobj_getattr(PObject *obj,uint16_t name);

//void pobj_print(Stream *ps, PObject *obj);


#define DBG_OBJ_BEGIN(o) dbg("{\"what\":\"obj\",\"at\":%-6i,\"ref\":%-6i,\"gc_flags\":%-2i,\"gc_size\":%-4i,\"type\":%-2i,\"flags\":%-3i",((uint32_t)o)-((uint32_t)hbase),o->header.gch.flags&0xfffffffc-((uint32_t)hbase),o->header.gch.flags&0x3,o->header.size,o->header.type,o->header.flags)
#define DBG_OBJ_END() dbg("},\n")

#define pobj_dump(ps,o) platform_stream_lock(ps); pobj_print(ps,o); platform_stream_unlock(ps)




#endif